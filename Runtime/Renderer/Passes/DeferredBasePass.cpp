#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "Renderer/Passes/DeferredBasePass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/VertexStreamCache.h"

constexpr EVertexAttributeFlags BASE_PASS_ATTRIBUTES =
    EVertexAttributeFlags::Position |
    EVertexAttributeFlags::TangentBasis |
    EVertexAttributeFlags::TexCoord0;

static bool GBasePassClearAllTargets = true;
static FAutoConsoleVariableRef CVarBasePassClearAllTargets(
    "Renderer.BasePass.ClearAllTargets",
    "Set to true to clear all the GBuffer RenderTargets inside of the BasePass, otherwise only a few targets are cleared to save bandwidth",
    GBasePassClearAllTargets);

static bool GBasePassBindless = false;
static FAutoConsoleVariableRef CVarBasePassBindless(
    "Renderer.BasePass.Bindless",
    "When true, the deferred BasePass samples material textures and samplers via SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] "
    "and a per-material indices buffer instead of conventional register bindings. Falls back to the static-binding variant if bindless is "
    "not supported by the RHI.",
    GBasePassBindless);

float GBasePassSpecularAAStrength = 1.0f;
static FAutoConsoleVariableRef CVarBasePassSpecularAAStrength(
    "Renderer.BasePass.SpecularAA.Strength",
    "Strength of the geometric specular anti-aliasing roughness gain applied in the deferred BasePass.",
    GBasePassSpecularAAStrength);

float GBasePassSpecularAAMaxRoughnessGain = 0.02f;
static FAutoConsoleVariableRef CVarBasePassSpecularAAMaxRoughnessGain(
    "Renderer.BasePass.SpecularAA.MaxRoughnessGain",
    "Maximum squared-roughness gain that geometric specular anti-aliasing can add in the deferred BasePass.",
    GBasePassSpecularAAMaxRoughnessGain);

class FNormalMap : SHADER_PERMUTATION_BOOL("ENABLE_NORMAL_MAPPING");

struct FBasePassShaderRules
{
    using FPermutation = TShaderPermutation<FMaterialPermutation, FNormalMap, FBindless>;

    static_assert(FPermutation::PermutationCount == 64, "BasePass permutation space grew unexpectedly");

    NODISCARD static FPermutation Create(const FMaterialFeatures& Features, bool bBindless)
    {
        FPermutation Permutation;
        Permutation.Set<FMaterialPermutation>(Features.CreatePermutation());
        Permutation.Set<FNormalMap>(Features.HasNormalMap());
        Permutation.Set<FBindless>(bBindless);
        return RemapPermutation(Permutation);
    }

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        return RemapMaterialPermutation(Permutation);
    }

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        if (Permutation.Get<FBindless>() && !Desc.bSupportsBindless)
        {
            return false;
        }

        return RemapPermutation(Permutation) == Permutation;
    }
};

class FBasePassVS : public FBasePassShaderRules
{
    DECLARE_SHADER_TYPE(FBasePassVS, EShaderStage::Vertex);
};

class FBasePassPS : public FBasePassShaderRules
{
    DECLARE_SHADER_TYPE(FBasePassPS, EShaderStage::Pixel);
};

IMPLEMENT_SHADER_TYPE(FBasePassVS, "Shaders/BasePass.hlsl", "VSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FBasePassPS, "Shaders/BasePass.hlsl", "PSMain", EShaderModel::SM_6_2);

NODISCARD static FGraphicsPipelineKey CreateBasePassPSOKey(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FBasePassShaderRules::FPermutation Permutation = FBasePassShaderRules::Create(Features, bBindless);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

FDeferredBasePass::FDeferredBasePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FDeferredBasePass::~FDeferredBasePass()
{
    MaterialPSOs.Clear();
}

void FDeferredBasePass::PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& /* FrameResources */)
{
    const FMaterialFeatures Features(Material, Declaration);

    const int32 MaterialFlags = static_cast<int32>(Features.Flags);
    const bool  bBindless     = RHI::bSupportsBindless && GBasePassBindless;

    const FGraphicsPipelineKey      PSOKey      = CreateBasePassPSOKey(Features, bBindless, Declaration);
    const FBasePassVS::FPermutation Permutation = FBasePassVS::FPermutation(PSOKey.PermutationID);

    FGraphicsPipelineStateInstance* CachedBasePassPSO = MaterialPSOs.Find(PSOKey);
    if (!CachedBasePassPSO)
    {
        FGraphicsPipelineStateInstance NewPipelineInstance;
        NewPipelineInstance.VertexShader = FShaderCache::Get().GetShader<FBasePassVS>(Permutation);
        if (!NewPipelineInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineInstance.PixelShader = FShaderCache::Get().GetShader<FBasePassPS>(Permutation);
        if (!NewPipelineInstance.PixelShader)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = Features.IsDoubleSided() ? ECullMode::None : ECullMode::Back;

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = EGBufferIndex::NumRenderTargets;

        FRHIRasterizerStateRef   RasterizerState   = RHI::CreateRasterizerState(RasterizerStateDesc);
        FRHIBlendStateRef        BlendState        = RHI::CreateBlendState(BlendStateDesc);
        FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);

        if (!DepthStencilState || !RasterizerState || !BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineInstance.StreamBinding = FVertexStreamCache::Get().GetBinding(Declaration, BASE_PASS_ATTRIBUTES);
        if (!NewPipelineInstance.StreamBinding)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                    = NewPipelineInstance.StreamBinding->InputLayout.Get();
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = NewPipelineInstance.VertexShader.Get();
        PSODesc.PixelShader                                    = NewPipelineInstance.PixelShader.Get();
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RendererTextureFormats::AlbedoFormat;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[1] = RendererTextureFormats::NormalFormat;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[2] = RendererTextureFormats::MaterialFormat;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[3] = RendererTextureFormats::VelocityFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = EGBufferIndex::NumRenderTargets;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;
        PSODesc.MultiSampleState.bProgrammableSamplePositions  = RHI::bSupportsProgrammableSamplePositions;

        NewPipelineInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!NewPipelineInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const String DebugName = String::Printf("BasePass PipelineState%s %d", bBindless ? " [Bindless]" : "", MaterialFlags);
            NewPipelineInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(PSOKey, Move(NewPipelineInstance));
    }
}

bool FDeferredBasePass::Initialize(FFrameResources& FrameResources)
{
    UNREFERENCED_VARIABLE(FrameResources);
    return true;
}

void FDeferredBasePass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("GBuffer", ERenderGraphPassFlags::Raster, GBasePassEnabled,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.SetRenderTarget(0, Context.GBufferAlbedo, EAttachmentLoadAction::Clear);
            PassBuilder.SetRenderTarget(1, Context.GBufferNormal, EAttachmentLoadAction::Clear);
            PassBuilder.SetRenderTarget(2, Context.GBufferMaterial, EAttachmentLoadAction::Clear);
            PassBuilder.SetRenderTarget(3, Context.GBufferVelocity, EAttachmentLoadAction::Clear);
            PassBuilder.SetDepthStencil(Context.GBufferDepth, EAttachmentLoadAction::Load, EAttachmentStoreAction::Store, FDepthStencilValue(1.0f, 0), false);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            Record(PassCommandList, Context.CreatePassResources(PassResources), Context.Scene);
        });
}

void FDeferredBasePass::Record(FRHICommandList& CommandList, const FPassResources& PassResources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, "Deferred BasePass");

    TRACE_SCOPE("Deferred BasePass");

    GPU_TRACE_SCOPE(CommandList, "Deferred BasePass");

    const float RenderWidth  = float(PassResources.RenderWidth);
    const float RenderHeight = float(PassResources.RenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const FFrameResources& FrameResources = *PassResources.FrameResources;
    const bool bBindless =
        RHI::bSupportsBindless &&
        GBasePassBindless &&
        FrameResources.MaterialDataBufferSRV.IsValid();

    for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        if (Material->ShouldRenderInForwardPass())
        {
            continue;
        }

        const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

        const FGraphicsPipelineKey PSOKey = CreateBasePassPSOKey(Features, bBindless, Batch.Declaration);

        FGraphicsPipelineStateInstance* PipelineInstance = MaterialPSOs.Find(PSOKey);
        if (!PipelineInstance)
        {
            DEBUG_BREAK();
            continue;
        }

        FRHIGraphicsPipelineState* PipelineState = PipelineInstance->PipelineState.Get();
        CHECK(PipelineState  != nullptr);
        CommandList.SetGraphicsPipelineState(PipelineState);

        CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        if (FRHIPixelShader* PixelShader = PipelineInstance->PixelShader.Get())
        {
            struct FBasePassConstants
            {
                float SpecularAAStrength;
                float SpecularAAMaxRoughnessGain;
                float Padding0;
                float Padding1;
            } BasePassConstants;

            BasePassConstants.SpecularAAStrength         = GBasePassSpecularAAStrength;
            BasePassConstants.SpecularAAMaxRoughnessGain = GBasePassSpecularAAMaxRoughnessGain;
            BasePassConstants.Padding0                   = 0.0f;
            BasePassConstants.Padding1                   = 0.0f;

            constexpr uint32 NumConstants = sizeof(FBasePassConstants) / sizeof(uint32);
            CommandList.SetShaderConstants(PixelShader, &BasePassConstants, NumConstants);
        }

        if (!bBindless)
        {
            BindMaterialTextures(CommandList, PipelineInstance->PixelShader.Get(), Features, *Material, 0);
        }

        CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 7);

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;

            StaticMesh->Mesh->SetVertexBuffers(CommandList, *PipelineInstance->StreamBinding);
            CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

            const int32 MaxMaterialIndex = Math::Max<int32>(int32(FrameResources.MaterialData.Size()) - 1, 0);
            StaticMesh->PerObjectBuffer.MaterialIndex = uint32(Math::Clamp<int32>(Material->GetBufferIndex(), 0, MaxMaterialIndex));
            CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);

            CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.PerObjectBuffer.Get(), 1);
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.PerObjectBuffer.Get(), 1);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }
}
