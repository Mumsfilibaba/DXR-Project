#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/DeferredRendering.h"
#include "Renderer/MaterialBindless.h"
#include "Renderer/PrePassShaders.h"
#include "Renderer/ReflectionSettings.h"
#include "Renderer/RenderFeatureSettings.h"
#include "Renderer/ShadowSettings.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/VertexStreamCache.h"

constexpr EVertexAttributeFlags BASE_PASS_ATTRIBUTES =
    EVertexAttributeFlags::Position | EVertexAttributeFlags::TangentBasis | EVertexAttributeFlags::TexCoord0;

static bool GDrawTileDebug = false;
static FAutoConsoleVariableRef CVarDrawTileDebug(
    "Renderer.Debug.DrawTiledLightning",
    "Draws the tiled lightning overlay, that displays how many lights are used in a certain tile",
    GDrawTileDebug);

static bool GBasePassClearAllTargets = true;
static FAutoConsoleVariableRef CVarBasePassClearAllTargets(
    "Renderer.BasePass.ClearAllTargets",
    "Set to true to clear all the GBuffer RenderTargets inside of the BasePass, otherwise only a few targets are cleared to save bandwidth",
    GBasePassClearAllTargets);

static bool GBasePassBindless = false;
static FAutoConsoleVariableRef CVarBasePassBindless(
    "Renderer.BasePass.Bindless",
    "When true, the deferred BasePass samples material textures and samplers via SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] and a per-material indices buffer instead of conventional register bindings. Falls back to the static-binding variant if bindless is not supported by the RHI.",
    GBasePassBindless);

bool GPrePassBindless = false;
static FAutoConsoleVariableRef CVarPrePassBindless(
    "Renderer.PrePass.Bindless",
    "When true, the depth pre-pass samples Albedo (alpha mask) and Height (parallax) via SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] and a per-material indices buffer instead of register bindings.",
    GPrePassBindless);

static float GIndirectSpecularStrength = 1.0f;
static FAutoConsoleVariableRef CVarIndirectSpecularStrength(
    "Renderer.Reflections.IndirectSpecularStrength",
    "Scalar multiplier applied to the indirect specular (IBL / ray-traced reflection) contribution in the deferred light pass.",
    GIndirectSpecularStrength);

static float GBasePassSpecularAAStrength = 1.0f;
static FAutoConsoleVariableRef CVarBasePassSpecularAAStrength(
    "Renderer.BasePass.SpecularAA.Strength",
    "Strength of the geometric specular anti-aliasing roughness gain applied in the deferred BasePass.",
    GBasePassSpecularAAStrength);

static float GBasePassSpecularAAMaxRoughnessGain = 0.02f;
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

class FBRDFIntegrationCS
{
    DECLARE_SHADER_TYPE(FBRDFIntegrationCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBRDFIntegrationCS, "Shaders/BRDFIntegationGen.hlsl", "Main", EShaderModel::SM_6_2);

enum class ETiledLightDebugMode : uint8
{
    None    = 0,
    Tiles   = 1,
    Cascade = 2,

    Count,
};

class FTiledLightDebug : SHADER_PERMUTATION_ENUM("TILED_LIGHT_DEBUG_MODE", ETiledLightDebugMode);

class FDeferredLightPassCS
{
    DECLARE_SHADER_TYPE(FDeferredLightPassCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<FTiledLightDebug>;
};

IMPLEMENT_SHADER_TYPE(FDeferredLightPassCS, "Shaders/DeferredLightPass.hlsl", "Main", EShaderModel::SM_6_2);

class FDepthReductionInitialCS
{
    DECLARE_SHADER_TYPE(FDepthReductionInitialCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

class FDepthReductionCS
{
    DECLARE_SHADER_TYPE(FDepthReductionCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FDepthReductionInitialCS, "Shaders/DepthReduction.hlsl", "ReductionMainInital", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FDepthReductionCS,        "Shaders/DepthReduction.hlsl", "ReductionMain",       EShaderModel::SM_6_2);

NODISCARD static FGraphicsPipelineKey CreatePrePassPSOKey(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FPrePassShaderRules::FPermutation Permutation = FPrePassShaderRules::Create(Features, bBindless, false);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

NODISCARD static FGraphicsPipelineKey CreateBasePassPSOKey(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FBasePassShaderRules::FPermutation Permutation = FBasePassShaderRules::Create(Features, bBindless);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

FDepthPrePass::FDepthPrePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FDepthPrePass::~FDepthPrePass()
{
    MaterialPSOs.Clear();
}

void FDepthPrePass::PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& /* FrameResources */)
{
    const FMaterialFeatures Features(Material, Declaration);

    const int32 MaterialFlags = static_cast<int32>(Features.Flags);
    const bool  bBindless     = RHI::bSupportsBindless && GPrePassBindless;

    const FGraphicsPipelineKey     PSOKey      = CreatePrePassPSOKey(Features, bBindless, Declaration);
    const FPrePassVS::FPermutation Permutation = FPrePassVS::FPermutation(PSOKey.PermutationID);

    FGraphicsPipelineStateInstance* CachedPrePassPSO = MaterialPSOs.Find(PSOKey);
    if (!CachedPrePassPSO)
    {
        FGraphicsPipelineStateInstance NewPipelineInstance;
        NewPipelineInstance.VertexShader = FShaderCache::Get().GetShader<FPrePassVS>(Permutation);

        if (!NewPipelineInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        // Nothing can discard without a height or alpha map, so the depth-only case needs no pixel shader.
        const bool bWantPixelShader = Features.HasHeightMap() || Features.HasAlphaMask();
        if (bWantPixelShader)
        {
            NewPipelineInstance.PixelShader = FShaderCache::Get().GetShader<FPrePassPS>(Permutation);
            if (!NewPipelineInstance.PixelShader)
            {
                DEBUG_BREAK();
                return;
            }
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = true;

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = Features.IsDoubleSided() ? ECullMode::None : ECullMode::Back;

        FRHIBlendStateDesc BlendStateDesc;

        FRHIRasterizerStateRef   RasterizerState   = RHI::CreateRasterizerState(RasterizerStateDesc);
        FRHIBlendStateRef        BlendState        = RHI::CreateBlendState(BlendStateDesc);
        FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);

        if (!DepthStencilState || !RasterizerState || !BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineInstance.StreamBinding = FVertexStreamCache::Get().GetBinding(Declaration, Features.GetDepthOnlyAttributes());
        if (!NewPipelineInstance.StreamBinding)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                   = NewPipelineInstance.StreamBinding->InputLayout.Get();
        PSODesc.BlendState                                    = BlendState.Get();
        PSODesc.DepthStencilState                             = DepthStencilState.Get();
        PSODesc.RasterizerState                               = RasterizerState.Get();
        PSODesc.VertexShader                                  = NewPipelineInstance.VertexShader.Get();
        PSODesc.PixelShader                                   = NewPipelineInstance.PixelShader.Get();
        PSODesc.RasterizerOutputFormats.DepthStencilFormat    = RendererTextureFormats::DepthBufferFormat;
        PSODesc.MultiSampleState.bProgrammableSamplePositions = RHI::bSupportsProgrammableSamplePositions;

        NewPipelineInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!NewPipelineInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const String DebugName = String::CreateFormatted("PrePass PipelineState%s %d",
                bBindless ? " [Bindless]" : "",
                MaterialFlags);
            NewPipelineInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(PSOKey, Move(NewPipelineInstance));
    }
}

bool FDepthPrePass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FDepthPrePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    // The prepass and base pass may rasterize this depth buffer with the hardware TAA jitter.
    ETextureUsageFlags Usage = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture;
    if (RHI::bSupportsProgrammableSamplePositions)
    {
        Usage |= ETextureUsageFlags::SamplePositionsCompatible;
    }

    const FClearValue DepthClearValue(RendererTextureFormats::DepthBufferFormat, 1.0f, 0);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::DepthBufferFormat, Width, Height, 1, 1, Usage, DepthClearValue);
    FrameResources.GBuffer[EGBufferIndex::Depth] = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);

    if (FrameResources.GBuffer[EGBufferIndex::Depth])
    {
        FrameResources.GBuffer[EGBufferIndex::Depth]->SetDebugName("GBuffer DepthStencil");
    }
    else
    {
        return false;
    }

    return true;
}

void FDepthPrePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, "Depth Pre-Pass");

    TRACE_SCOPE("Depth Pre-Pass");

    GPU_TRACE_SCOPE(CommandList, "Depth Pre-Pass");

    FRHIDepthStencilView* DepthStencilView = FrameResources.GBuffer[EGBufferIndex::Depth]->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView);

    CommandList.BeginRenderPass(RenderPassDesc);

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const bool bBindless = RHI::bSupportsBindless && GPrePassBindless && FrameResources.MaterialDataBufferSRV.IsValid();

    for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        CHECK(Material != nullptr);

        if (!Material->ShouldRenderInPrePass())
        {
            continue;
        }

        const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

        const FGraphicsPipelineKey PSOKey = CreatePrePassPSOKey(Features, bBindless, Batch.Declaration);

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

        if (Features.HasHeightMap())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        }

        if (Features.HasAlphaMask() || Features.HasHeightMap())
        {
            if (bBindless)
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 2);
            }
            else
            {
                if (Features.HasHeightMap())
                {
                    CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 2);
                }

                CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                if (Features.HasAlphaMask())
                {
                    CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
                }

                if (Features.HasHeightMap())
                {
                    CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }
            }
        }

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
            StaticMesh->Mesh->SetVertexBuffers(CommandList, *PipelineInstance->StreamBinding);

            CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

            const int32 MaxMaterialIndex = Math::Max<int32>(int32(FrameResources.MaterialData.Size()) - 1, 0);
            StaticMesh->PerObjectBuffer.MaterialIndex = uint32(Math::Clamp<int32>(Material->GetBufferIndex(), 0, MaxMaterialIndex));

            CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);
            CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.PerObjectBuffer.Get(), 1);

            if (FRHIPixelShader* PixelShader = PipelineInstance->PixelShader.Get())
            {
                CommandList.SetConstantBuffer(PixelShader, FrameResources.PerObjectBuffer.Get(), 1);
            }

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();
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
            const String DebugName = String::CreateFormatted("BasePass PipelineState%s %d",
                bBindless ? " [Bindless]" : "",
                MaterialFlags);
            NewPipelineInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(PSOKey, Move(NewPipelineInstance));
    }
}

bool FDeferredBasePass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FDeferredBasePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::AlbedoFormat, Width, Height, 1, 1, Usage);

    // Albedo
    FrameResources.GBuffer[EGBufferIndex::Albedo] = RHI::CreateTexture(TextureDesc, ERHIResourceState::NonPixelShaderResource);
    if (FrameResources.GBuffer[EGBufferIndex::Albedo])
    {
        FrameResources.GBuffer[EGBufferIndex::Albedo]->SetDebugName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    TextureDesc.Format = RendererTextureFormats::NormalFormat;

    FrameResources.GBuffer[EGBufferIndex::Normal] = RHI::CreateTexture(TextureDesc, ERHIResourceState::NonPixelShaderResource);
    if (FrameResources.GBuffer[EGBufferIndex::Normal])
    {
        FrameResources.GBuffer[EGBufferIndex::Normal]->SetDebugName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    TextureDesc.Format = RendererTextureFormats::MaterialFormat;

    FrameResources.GBuffer[EGBufferIndex::Material] = RHI::CreateTexture(TextureDesc, ERHIResourceState::NonPixelShaderResource);
    if (FrameResources.GBuffer[EGBufferIndex::Material])
    {
        FrameResources.GBuffer[EGBufferIndex::Material]->SetDebugName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // Velocity
    TextureDesc.Format = RendererTextureFormats::VelocityFormat;

    FrameResources.GBuffer[EGBufferIndex::Velocity] = RHI::CreateTexture(TextureDesc, ERHIResourceState::NonPixelShaderResource);
    if (FrameResources.GBuffer[EGBufferIndex::Velocity])
    {
        FrameResources.GBuffer[EGBufferIndex::Velocity]->SetDebugName("GBuffer Velocity");
    }
    else
    {
        return false;
    }

    return true;
}

void FDeferredBasePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, "Deferred BasePass");

    TRACE_SCOPE("Deferred BasePass");

    GPU_TRACE_SCOPE(CommandList, "Deferred BasePass");

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    const EAttachmentLoadAction LoadAction = GBasePassClearAllTargets ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load;

    FRHIRenderTargetView* AlbedoRenderTargetView   = FrameResources.GBuffer[EGBufferIndex::Albedo]->GetRenderTargetView();
    FRHIRenderTargetView* NormalRenderTargetView   = FrameResources.GBuffer[EGBufferIndex::Normal]->GetRenderTargetView();
    FRHIRenderTargetView* MaterialRenderTargetView = FrameResources.GBuffer[EGBufferIndex::Material]->GetRenderTargetView();
    FRHIRenderTargetView* VelocityRenderTargetView = FrameResources.GBuffer[EGBufferIndex::Velocity]->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView         = FrameResources.GBuffer[EGBufferIndex::Depth]->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets                       = EGBufferIndex::NumRenderTargets;
    RenderPassDesc.RenderTargets[EGBufferIndex::Albedo]   = FRHIRenderPassAttachment(AlbedoRenderTargetView, LoadAction);
    RenderPassDesc.RenderTargets[EGBufferIndex::Normal]   = FRHIRenderPassAttachment(NormalRenderTargetView, EAttachmentLoadAction::Clear);
    RenderPassDesc.RenderTargets[EGBufferIndex::Material] = FRHIRenderPassAttachment(MaterialRenderTargetView, LoadAction);
    RenderPassDesc.RenderTargets[EGBufferIndex::Velocity] = FRHIRenderPassAttachment(VelocityRenderTargetView, LoadAction);
    RenderPassDesc.DepthStencilAttachment                 = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const bool bBindless = RHI::bSupportsBindless && GBasePassBindless && FrameResources.MaterialDataBufferSRV.IsValid();

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

        if (bBindless)
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 4);
        }
        else
        {
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);

            if (Features.HasNormalMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->NormalMap->GetShaderResourceView(), 1);
            }

            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->MaterialMap->GetShaderResourceView(), 2);

            if (Features.HasHeightMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 3);
            }

            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);

            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 4);

            CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
        }

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

    CommandList.EndRenderPass();
}

FTiledLightPass::FTiledLightPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , TiledLightPassPSO(nullptr)
    , TiledLightShader(nullptr)
    , TiledLightPassPSO_TileDebug(nullptr)
    , TiledLightShader_TileDebug(nullptr)
    , TiledLightPassPSO_CascadeDebug(nullptr)
    , TiledLightShader_CascadeDebug(nullptr)
{
}

FTiledLightPass::~FTiledLightPass()
{
    TiledLightPassPSO.Reset();
    TiledLightShader.Reset();
    TiledLightPassPSO_TileDebug.Reset();
    TiledLightShader_TileDebug.Reset();
    TiledLightPassPSO_CascadeDebug.Reset();
    TiledLightShader_CascadeDebug.Reset();
}

bool FTiledLightPass::Initialize(FFrameResources& FrameResources)
{
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.GBufferSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!FrameResources.GBufferSampler)
    {
        return false;
    }

    if (!CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight))
    {
        return false;
    }

    constexpr uint32  LUTSize   = 512;
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;

    if (!RHI::Device->QueryUAVFormatSupport(LUTFormat))
    {
        LOG_ERROR("[FSceneRenderer]: R16G16_Float is not supported for UAVs");
        return false;
    }

    FRHITextureDesc LUTDesc = FRHITextureDesc::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);
    FRHITextureRef StagingTexture = RHI::CreateTexture(LUTDesc, ERHIResourceState::Common);

    if (!StagingTexture)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        StagingTexture->SetDebugName("Staging IntegrationLUT");
    }

    LUTDesc.UsageFlags = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest;

    FrameResources.IntegrationLUT = RHI::CreateTexture(LUTDesc, ERHIResourceState::Common);
    if (!FrameResources.IntegrationLUT)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetDebugName("IntegrationLUT");
    }

    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!FrameResources.IntegrationLUTSampler)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputeShaderRef BRDFShader = FShaderCache::Get().GetShader<FBRDFIntegrationCS>();
    if (!BRDFShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = BRDFShader.Get();

    FRHIComputePipelineStateRef BRDFPipelineState = RHI::CreateComputePipelineState(PSODesc);
    if (!BRDFPipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BRDFPipelineState->SetDebugName("BRDFIntegationGen PipelineState");
    }

    FRHICommandList CommandList;
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));

    CommandList.SetComputePipelineState(BRDFPipelineState.Get());

    FRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(BRDFShader.Get(), StagingUAV, 0);

    constexpr uint32 ThreadCount    = 16;
    constexpr uint32 DispatchWidth  = Math::DivideByMultiple(LUTSize, ThreadCount);
    constexpr uint32 DispatchHeight = Math::DivideByMultiple(LUTSize, ThreadCount);

    CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CommandList.UnorderedAccessBarrier(StagingTexture.Get());

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.IntegrationLUT.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));

    CommandList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(FrameResources.IntegrationLUT.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    // Tiled lightning
    FDeferredLightPassCS::FPermutation LightPassPermutation;
    LightPassPermutation.Set<FTiledLightDebug>(ETiledLightDebugMode::None);

    TiledLightShader = FShaderCache::Get().GetShader<FDeferredLightPassCS>(LightPassPermutation);
    if (!TiledLightShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc DeferredLightPassPSODesc;
    DeferredLightPassPSODesc.Shader = TiledLightShader.Get();

    TiledLightPassPSO = RHI::CreateComputePipelineState(DeferredLightPassPSODesc);
    if (!TiledLightPassPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO->SetDebugName("DeferredLightPass PipelineState");
    }

    // Tiled lightning Tile debugging
    LightPassPermutation.Set<FTiledLightDebug>(ETiledLightDebugMode::Tiles);

    TiledLightShader_TileDebug = FShaderCache::Get().GetShader<FDeferredLightPassCS>(LightPassPermutation);
    if (!TiledLightShader_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassPSODesc.Shader = TiledLightShader_TileDebug.Get();

    TiledLightPassPSO_TileDebug = RHI::CreateComputePipelineState(DeferredLightPassPSODesc);
    if (!TiledLightPassPSO_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_TileDebug->SetDebugName("DeferredLightPass PipelineState Tile-Debug");
    }

    // Tiled lightning Cascade debugging
    LightPassPermutation.Set<FTiledLightDebug>(ETiledLightDebugMode::Cascade);

    TiledLightShader_CascadeDebug = FShaderCache::Get().GetShader<FDeferredLightPassCS>(LightPassPermutation);
    if (!TiledLightShader_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassPSODesc.Shader = TiledLightShader_CascadeDebug.Get();

    TiledLightPassPSO_CascadeDebug = RHI::CreateComputePipelineState(DeferredLightPassPSODesc);
    if (!TiledLightPassPSO_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_CascadeDebug->SetDebugName("DeferredLightPass PipelineState Cascade-Debug");
    }

    return true;
}

bool FTiledLightPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopySource;
    FRHITextureDesc SceneTargetDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::SceneTargetFormat, Width, Height, 1, 1, Usage);
    FrameResources.SceneTarget = RHI::CreateTexture(SceneTargetDesc, ERHIResourceState::PixelShaderResource);
    
    if (FrameResources.SceneTarget)
    {
        FrameResources.SceneTarget->SetDebugName("Scene Target");
    }
    else
    {
        return false;
    }

    return true;
}

void FTiledLightPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    if (FrameResources.CurrentRenderWidth == 0 || FrameResources.CurrentRenderHeight == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "LightPass");

    TRACE_SCOPE("LightPass");

    GPU_TRACE_SCOPE(CommandList, "Light Pass");

    const bool bDrawCascades = GCSMDebugCascades;

    FRHIComputeShader* LightPassShader;
    if (GDrawTileDebug)
    {
        LightPassShader = TiledLightShader_TileDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_TileDebug.Get());
    }
    else if (bDrawCascades)
    {
        LightPassShader = TiledLightShader_CascadeDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_CascadeDebug.Get());
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO.Get());
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 3);

    const bool bUseRayTracingReflections = RHI::bSupportsRayTracing && GRayTracingEnabled && GReflectionsEnabled && FrameResources.RayTracingOutput;
    if (bUseRayTracingReflections)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.RayTracingOutput.Get(), ERHIResourceState::NonPixelShaderResource));
        CommandList.SetShaderResourceView(LightPassShader, FrameResources.RayTracingOutput->GetShaderResourceView(), 4);
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 5);

    if (Scene)
    {
        // Global SkyLight as a fallback
        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
        {
            CommandList.SetShaderResourceView(LightPassShader, SkyLight->DiffuseCubeMap->GetShaderResourceView(), 6);
            CommandList.SetShaderResourceView(LightPassShader, SkyLight->SpecularCubeMap->GetShaderResourceView(), 7);
        }

        // Local Light-Probe
        if (!Scene->GetLightProbes().IsEmpty())
        {
            // TODO: Support more than the first probe
            if (FSceneLightProbe* LightProbe = Scene->GetLightProbes().First())
            {
                CommandList.SetShaderResourceView(LightPassShader, LightProbe->DiffuseCubeMap->GetShaderResourceView(), 8);
                CommandList.SetShaderResourceView(LightPassShader, LightProbe->SpecularCubeMap->GetShaderResourceView(), 9);
            }
        }
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.DirectionalShadowMask->GetShaderResourceView(), 10);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.PointLightShadowMaps->GetShaderResourceView(), 11);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 12);

    if (bDrawCascades)
    {
        CommandList.SetShaderResourceView(LightPassShader, FrameResources.CascadeIndexBuffer->GetShaderResourceView(), 13);
    }

    CommandList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsBuffer.Get(), 1);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.DirectionalLightDataBuffer.Get(), 5);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.LightProbeBuffer.Get(), 6);

    CommandList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
    CommandList.SetSamplerState(LightPassShader, FrameResources.LightProbeSampler.Get(), 1);
    CommandList.SetSamplerState(LightPassShader, FrameResources.GBufferSampler.Get(), 2);
    CommandList.SetSamplerState(LightPassShader, FrameResources.PointLightShadowSampler.Get(), 3);

    FRHIUnorderedAccessView* SceneTargetUAV = FrameResources.SceneTarget->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(LightPassShader, SceneTargetUAV, 0);

    struct FLightPassSettingsHLSL
    {
        // 0-16
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 NumLightProbes;

        // 16-32
        int32 ScreenWidth;
        int32 ScreenHeight;
        int32 bEnablePointLightShadows;
        int32 bEnableRayTracingReflections;

        // 32-48
        float IndirectSpecularStrength;
    } LightPassSettings;

    const int32 RenderWidth  = FrameResources.CurrentRenderWidth;
    const int32 RenderHeight = FrameResources.CurrentRenderHeight;

    LightPassSettings.NumSkyLightMips              = 0;
    LightPassSettings.NumShadowCastingPointLights  = FrameResources.ShadowCastingPointLightsData.Size();
    LightPassSettings.NumPointLights               = FrameResources.PointLightsData.Size();
    LightPassSettings.NumLightProbes               = FrameResources.LightProbeInfos.Size();
    LightPassSettings.ScreenWidth                  = static_cast<int32>(RenderWidth);
    LightPassSettings.ScreenHeight                 = static_cast<int32>(RenderHeight);
    LightPassSettings.bEnableRayTracingReflections = bUseRayTracingReflections ? 1 : 0;
    LightPassSettings.IndirectSpecularStrength     = GIndirectSpecularStrength;

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
        {
            LightPassSettings.NumSkyLightMips = SkyLight->SpecularCubeMap->GetDesc().NumMipLevels;
        }
    }

    // Point-light shadows also require the master shadow toggle
    LightPassSettings.bEnablePointLightShadows = (GPointLightShadowsEnabled && GShadowsEnabled) ? 1 : 0;

    constexpr uint32 NumConstants = sizeof(FLightPassSettingsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(LightPassShader, &LightPassSettings, NumConstants);

    constexpr uint32 NumThreads = 16;
    const uint32 WorkGroupWidth  = Math::DivideByMultiple<uint32>(LightPassSettings.ScreenWidth, NumThreads);
    const uint32 WorkGroupHeight = Math::DivideByMultiple<uint32>(LightPassSettings.ScreenHeight, NumThreads);
    CommandList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);
}

FDepthReducePass::FDepthReducePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , ReduceDepthInitalPSO(nullptr)
    , ReduceDepthInitalShader(nullptr)
    , ReduceDepthPSO(nullptr)
    , ReduceDepthShader(nullptr)
{
}

FDepthReducePass::~FDepthReducePass()
{
    ReduceDepthInitalPSO.Reset();
    ReduceDepthInitalShader.Reset();
    ReduceDepthPSO.Reset();
    ReduceDepthShader.Reset();
}

bool FDepthReducePass::Initialize(FFrameResources& FrameResources)
{
    // Depth-Reduction
    ReduceDepthInitalShader = FShaderCache::Get().GetShader<FDepthReductionInitialCS>();
    if (!ReduceDepthInitalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = ReduceDepthInitalShader.Get();

    ReduceDepthInitalPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!ReduceDepthInitalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthInitalPSO->SetDebugName("Initial DepthReduction PipelineState");
    }

    // Depth-Reduction
    ReduceDepthShader = FShaderCache::Get().GetShader<FDepthReductionCS>();
    if (!ReduceDepthShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = ReduceDepthShader.Get();

    ReduceDepthPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!ReduceDepthPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthPSO->SetDebugName("DepthReduction PipelineState");
    }

    if (!CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight))
    {
        return false;
    }

    return true;
}

bool FDepthReducePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    constexpr uint32 Alignment = 16;
    const uint32 ReducedWidth  = Math::DivideByMultiple(Width, Alignment);
    const uint32 ReducedHeight = Math::DivideByMultiple(Height, Alignment);

    const ETextureUsageFlags Usage = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32_Float, ReducedWidth, ReducedHeight, 1, 1, Usage);
    for (int32 Index = 0; Index < FrameResources.NumReducedDepthBuffers; Index++)
    {
        FrameResources.ReducedDepthBuffer[Index] = RHI::CreateTexture(TextureDesc, ERHIResourceState::NonPixelShaderResource);
        if (FrameResources.ReducedDepthBuffer[Index])
        {
            FrameResources.ReducedDepthBuffer[Index]->SetDebugName("Reduced DepthStencil[" + TTypeToString<int32>::ToString(Index) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

void FDepthReducePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    if (FrameResources.ReducedDepthBuffer[0]->GetDesc().Extent.X == 0 || FrameResources.ReducedDepthBuffer[0]->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "Depth Reduction");

    TRACE_SCOPE("Depth Reduction");

    GPU_TRACE_SCOPE(CommandList, "Depth Reduction");

    struct FReductionConstants
    {
        Matrix4 CamProjection;
        float   NearPlane;
        float   FarPlane;
    } ReductionConstants;

    FSceneCamera* Camera = Scene->GetCamera();
    ReductionConstants.CamProjection = Camera->Snapshot.Projection;
    ReductionConstants.NearPlane     = Camera->Snapshot.NearPlane;
    ReductionConstants.FarPlane      = Camera->Snapshot.FarPlane;

    // Perform the first reduction
    const FRHITransitionBarrierDesc BeginReduction[] =
    {
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::DepthWrite, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.ReducedDepthBuffer[0].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess),
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.ReducedDepthBuffer[1].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess),
    };

    CommandList.TransitionBarrier(BeginReduction);

    CommandList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthInitalShader.Get(), FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    constexpr uint32 NumConstants = sizeof(FReductionConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, NumConstants);

    uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetDesc().Extent.X;
    uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetDesc().Extent.Y;
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    const FRHITransitionBarrierDesc FinishFirstReduction[] =
    {
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.ReducedDepthBuffer[0].Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::DepthWrite),
    };

    CommandList.TransitionBarrier(FinishFirstReduction);

    // Perform the other reductions
    CommandList.SetComputePipelineState(ReduceDepthPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0);

    ThreadsX = Math::DivideByMultiple(ThreadsX, 16);
    ThreadsY = Math::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    const FRHITransitionBarrierDesc SwapReductionTargets[] =
    {
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.ReducedDepthBuffer[0].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess),
        FRHITransitionBarrierDesc::CreateTexture(FrameResources.ReducedDepthBuffer[1].Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource),
    };

    CommandList.TransitionBarrier(SwapReductionTargets);

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    ThreadsX = Math::DivideByMultiple(ThreadsX, 16);
    ThreadsY = Math::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.ReducedDepthBuffer[0].Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
}
