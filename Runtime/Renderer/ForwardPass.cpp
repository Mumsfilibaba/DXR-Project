#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/VertexStreamCache.h"

static constexpr EVertexAttributeFlags FORWARD_PASS_ATTRIBUTES = 
    EVertexAttributeFlags::Position | EVertexAttributeFlags::TangentBasis | EVertexAttributeFlags::TexCoord0;

static bool GForwardPassBindless = false;
static FAutoConsoleVariableRef CVarForwardPassBindless(
    "Renderer.ForwardPass.Bindless",
    "When true, the forward pass samples per-material textures (Albedo / Normal / Material / Height) and the material sampler via SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] and a per-material indices buffer instead of register bindings. Full-frame SRVs / samplers (sky, integration LUT, shadow maps) remain non-bindless.",
    GForwardPassBindless);

struct FForwardPassShaderRules
{
    using FPermutation = TShaderPermutation<FParallax, FClipping, FBindless>;

    static_assert(FPermutation::PermutationCount == 8, "ForwardPass permutation space grew unexpectedly");

    NODISCARD static FPermutation Create(bool bBindless, bool bEnableParallax, bool bEnableClipping)
    {
        FPermutation Permutation;
        Permutation.Set<FParallax>(bEnableParallax);
        Permutation.Set<FClipping>(bEnableClipping);
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

    static void ModifyCompilationEnvironment(const FShaderPermutationDesc&, FShaderCompilationEnvironment& Environment)
    {
        Environment.SetDefine("ENABLE_NORMAL_MAPPING", "(1)");
    }
};

class FForwardPassVS : public FForwardPassShaderRules
{
    DECLARE_SHADER_TYPE(FForwardPassVS, EShaderStage::Vertex);
};

class FForwardPassPS : public FForwardPassShaderRules
{
    DECLARE_SHADER_TYPE(FForwardPassPS, EShaderStage::Pixel);
};

IMPLEMENT_SHADER_TYPE(FForwardPassVS, "Shaders/ForwardPass.hlsl", "VSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FForwardPassPS, "Shaders/ForwardPass.hlsl", "PSMain", EShaderModel::SM_6_2);

NODISCARD static FGraphicsPipelineKey CreateForwardPassPSOKey(bool bBindless, bool bEnableParallax, bool bEnableClipping, const FVertexDeclaration& Declaration)
{
    const FForwardPassShaderRules::FPermutation Permutation = FForwardPassShaderRules::Create(bBindless, bEnableParallax, bEnableClipping);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

FForwardPass::FForwardPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FForwardPass::~FForwardPass()
{
    PipelineStates.Clear();
}

FGraphicsPipelineStateInstance* FForwardPass::CompilePipelineState(bool bBindless, bool bEnableParallax, bool bEnableClipping, const FVertexDeclaration& Declaration)
{
    const FGraphicsPipelineKey         Key         = CreateForwardPassPSOKey(bBindless, bEnableParallax, bEnableClipping, Declaration);
    const FForwardPassVS::FPermutation Permutation = FForwardPassVS::FPermutation(Key.PermutationID);

    FGraphicsPipelineStateInstance NewInstance;

    NewInstance.VertexShader = FShaderCache::Get().GetShader<FForwardPassVS>(Permutation);
    if (!NewInstance.VertexShader)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    NewInstance.PixelShader = FShaderCache::Get().GetShader<FForwardPassPS>(Permutation);
    if (!NewInstance.PixelShader)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateDesc.bDepthEnable      = true;
    DepthStencilStateDesc.bDepthWriteEnable = true;

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;
    BlendStateDesc.RenderTargets[0].bBlendEnable = true;
    BlendStateDesc.RenderTargets[0].SrcBlend = EBlendType::One;
    BlendStateDesc.RenderTargets[0].DstBlend = EBlendType::Zero;

    FRHIRasterizerStateRef   RasterizerState   = RHI::CreateRasterizerState(RasterizerStateDesc);
    FRHIBlendStateRef        BlendState        = RHI::CreateBlendState(BlendStateDesc);
    FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);

    if (!DepthStencilState || !RasterizerState || !BlendState)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    NewInstance.StreamBinding = FVertexStreamCache::Get().GetBinding(Declaration, FORWARD_PASS_ATTRIBUTES);
    if (!NewInstance.StreamBinding)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.VertexShader                                   = NewInstance.VertexShader.Get();
    PSODesc.PixelShader                                    = NewInstance.PixelShader.Get();
    PSODesc.InputLayout                                    = NewInstance.StreamBinding->InputLayout.Get();
    PSODesc.DepthStencilState                              = DepthStencilState.Get();
    PSODesc.BlendState                                     = BlendState.Get();
    PSODesc.RasterizerState                                = RasterizerState.Get();
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RendererTextureFormats::SceneTargetFormat;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;

    NewInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
    if (!NewInstance.PipelineState)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    const String DebugName = String::CreateFormatted("ForwardPass PipelineState%s%s%s [Declaration %u]",
        bEnableParallax ? " [Parallax]" : "",
        bEnableClipping ? " [Clipping]" : "",
        bBindless ? " [Bindless]" : "",
        static_cast<uint32>(Declaration.GetID()));
    NewInstance.PipelineState->SetDebugName(DebugName);

    PipelineStates.Add(Key, Move(NewInstance));
    return PipelineStates.Find(Key);
}

bool FForwardPass::Initialize(FFrameResources& /* FrameResources */)
{
    struct FVariant
    {
        bool bEnableParallax;
        bool bEnableClipping;
    };

    const FVertexDeclaration& Declaration = FVertexDeclaration::GetStandardStaticMesh();

    const FVariant Variants[] = { { false, false }, { true, false }, { true, true } };
    for (const FVariant& Variant : Variants)
    {
        if (!CompilePipelineState(false, Variant.bEnableParallax, Variant.bEnableClipping, Declaration))
        {
            return false;
        }

        if (RHI::bSupportsBindless && !CompilePipelineState(true, Variant.bEnableParallax, Variant.bEnableClipping, Declaration))
        {
            return false;
        }
    }

    return true;
}

void FForwardPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    // Forward Pass
    RHI_EVENT_SCOPE(CommandList, "ForwardPass");

    TRACE_SCOPE("ForwardPass");

    GPU_TRACE_SCOPE(CommandList, "Forward Pass");

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.ShadowCascades.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIRenderTargetView* RenderTargetView = FrameResources.SceneTarget->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = FrameResources.GBuffer[EGBufferIndex::Depth]->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);
    CommandList.BeginRenderPass(RenderPassDesc);

    const bool bBindless = RHI::bSupportsBindless && GForwardPassBindless && FrameResources.MaterialDataBufferSRV.IsValid();

    const auto BindFrameResources = [&](FRHIPixelShader* PShader)
    {
        CommandList.SetConstantBuffer(PShader, FrameResources.CameraBuffer.Get(), 0);
        // TODO: Fix point-light count in shader
        //CmdList.SetConstantBuffer(PShader, LightSetup.PointLightsBuffer.Get(), 1);
        //CmdList.SetConstantBuffer(PShader, LightSetup.PointLightsPosRadBuffer.Get(), 2);
        CommandList.SetConstantBuffer(PShader, FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
        CommandList.SetConstantBuffer(PShader, FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
        CommandList.SetConstantBuffer(PShader, FrameResources.DirectionalLightDataBuffer.Get(), 5);

        if (Scene)
        {
            if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
            {
                CommandList.SetShaderResourceView(PShader, SkyLight->DiffuseCubeMap->GetShaderResourceView(), 0);
                CommandList.SetShaderResourceView(PShader, SkyLight->SpecularCubeMap->GetShaderResourceView(), 1);
            }
        }

        CommandList.SetShaderResourceView(PShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
        //TODO: Fix directional-light shadows
        //CmdList.SetShaderResourceView(PShader, LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 3);
        CommandList.SetShaderResourceView(PShader, FrameResources.PointLightShadowMaps->GetShaderResourceView(), 4);

        CommandList.SetSamplerState(PShader, FrameResources.IntegrationLUTSampler.Get(), 1);
        CommandList.SetSamplerState(PShader, FrameResources.LightProbeSampler.Get(), 2);
        CommandList.SetSamplerState(PShader, FrameResources.PointLightShadowSampler.Get(), 3);
        //CmdList.SetSamplerState(PShader, FrameResources.DirectionalLightShadowSampler.Get(), 4);
    };

    FGraphicsPipelineStateInstance* PipelineInstance = nullptr;
    FRHIVertexShaderRef             VShader;
    FRHIPixelShaderRef              PShader;

    for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        if (!Material->ShouldRenderInForwardPass())
        {
            continue;
        }

        const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

        const bool bEnableParallax = Features.HasHeightMap();
        const bool bEnableClipping = Features.HasParallaxClipping();

        const FGraphicsPipelineKey PSOKey = CreateForwardPassPSOKey(bBindless, bEnableParallax, bEnableClipping, Batch.Declaration);

        FGraphicsPipelineStateInstance* MaterialPipeline = PipelineStates.Find(PSOKey);
        if (!MaterialPipeline)
        {
            MaterialPipeline = CompilePipelineState(bBindless, bEnableParallax, bEnableClipping, Batch.Declaration);
        }

        if (!MaterialPipeline)
        {
            DEBUG_BREAK();
            continue;
        }

        if (MaterialPipeline != PipelineInstance)
        {
            PipelineInstance = MaterialPipeline;
            VShader          = PipelineInstance->VertexShader;
            PShader          = PipelineInstance->PixelShader;

            CommandList.SetGraphicsPipelineState(PipelineInstance->PipelineState.Get());
            BindFrameResources(PShader.Get());
        }

        CommandList.SetShaderResourceView(PShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 12);

        if (!bBindless)
        {
            BindMaterialTextures(CommandList, PShader.Get(), Features, *Material, 5);
        }

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;

            StaticMesh->Mesh->SetVertexBuffers(CommandList, *PipelineInstance->StreamBinding);
            CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

            StaticMesh->PerObjectBuffer.MaterialIndex = Material->GetBufferIndex();
            CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);

            CommandList.SetConstantBuffer(VShader.Get(), FrameResources.PerObjectBuffer.Get(), 6);
            CommandList.SetConstantBuffer(PShader.Get(), FrameResources.PerObjectBuffer.Get(), 6);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.ShadowCascades.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource));
}
