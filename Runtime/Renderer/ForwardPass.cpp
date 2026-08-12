#include "Core/Math/Math.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/ReflectionSettings.h"
#include "Renderer/RenderFeatureSettings.h"
#include "Renderer/ShadingSettings.h"
#include "Renderer/ShadowRendering.h"
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

class FTranslucent : SHADER_PERMUTATION_BOOL("ENABLE_TRANSLUCENT");
class FRefraction  : SHADER_PERMUTATION_BOOL("ENABLE_REFRACTION");

struct FForwardPassShaderRules
{
    using FPermutation = TShaderPermutation<FParallax, FClipping, FAlphaMask, FTranslucent, FRefraction, FBindless>;

    static_assert(FPermutation::PermutationCount == 64, "ForwardPass permutation space grew unexpectedly");

    NODISCARD static FPermutation Create(const FMaterialFeatures& Features, bool bBindless)
    {
        FPermutation Permutation;
        Permutation.Set<FParallax>(Features.HasHeightMap());
        Permutation.Set<FClipping>(Features.HasParallaxClipping());
        Permutation.Set<FAlphaMask>(Features.HasAlphaMask());
        Permutation.Set<FTranslucent>(Features.IsTranslucent());
        Permutation.Set<FRefraction>(Features.HasRefraction());
        Permutation.Set<FBindless>(bBindless);
        return RemapPermutation(Permutation);
    }

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        Permutation = RemapMaterialPermutation(Permutation);

        if (!Permutation.Get<FTranslucent>())
        {
            Permutation.Set<FRefraction>(false);
        }
        else
        {
            Permutation.Set<FAlphaMask>(false);
        }

        return Permutation;
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

NODISCARD static FGraphicsPipelineKey CreateForwardPassPSOKey(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FForwardPassShaderRules::FPermutation Permutation = FForwardPassShaderRules::Create(Features, bBindless);

    const uint32 PipelineFlags = Features.IsDoubleSided() ? PIPELINE_FLAG_DOUBLE_SIDED : 0u;
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), PipelineFlags, Declaration.GetID());
}

FForwardPass::FForwardPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , PipelineStates()
    , CachedReadOnlyDepthDSV(nullptr)
    , CachedReadOnlyDepthTarget(nullptr)
{
}

FForwardPass::~FForwardPass()
{
    PipelineStates.Clear();
}

FGraphicsPipelineStateInstance* FForwardPass::CompilePipelineState(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FGraphicsPipelineKey         Key         = CreateForwardPassPSOKey(Features, bBindless, Declaration);
    const FForwardPassVS::FPermutation Permutation = FForwardPassShaderRules::Create(Features, bBindless);

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
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = Features.IsDoubleSided() ? ECullMode::None : ECullMode::Back;

    const bool bTranslucent = Features.IsTranslucent();

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    if (bTranslucent)
    {
        BlendStateDesc.RenderTargets[0].bBlendEnable   = true;
        BlendStateDesc.RenderTargets[0].SrcBlend       = EBlendType::One;
        BlendStateDesc.RenderTargets[0].DstBlend       = EBlendType::InvSrcAlpha;
        BlendStateDesc.RenderTargets[0].BlendOp        = EBlendOp::Add;
        BlendStateDesc.RenderTargets[0].ColorWriteMask = EColorWriteFlags::Red | EColorWriteFlags::Green | EColorWriteFlags::Blue;
    }

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

    const String DebugName = String::Printf("ForwardPass PipelineState%s%s%s%s%s%s [Declaration %u]",
        Permutation.Get<FParallax>() ? " [Parallax]" : "",
        Permutation.Get<FClipping>() ? " [Clipping]" : "",
        Permutation.Get<FAlphaMask>() ? " [AlphaMask]" : "",
        Permutation.Get<FTranslucent>() ? " [Translucent]" : "",
        Permutation.Get<FRefraction>() ? " [Refraction]" : "",
        Permutation.Get<FBindless>() ? " [Bindless]" : "",
        static_cast<uint32>(Declaration.GetID()));
    NewInstance.PipelineState->SetDebugName(DebugName);

    PipelineStates.Add(Key, Move(NewInstance));
    return PipelineStates.Find(Key);
}

bool FForwardPass::Initialize(FFrameResources& /* FrameResources */)
{
    return true;
}

void FForwardPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    // Forward Pass
    RHI_EVENT_SCOPE(CommandList, "ForwardPass");

    TRACE_SCOPE("ForwardPass");

    GPU_TRACE_SCOPE(CommandList, "Forward Pass");

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FRHITexture*          DepthTarget      = FrameResources.GBuffer[EGBufferIndex::Depth].Get();
    FRHIRenderTargetView* RenderTargetView = FrameResources.SceneTarget->GetRenderTargetView();

    if (CachedReadOnlyDepthTarget != DepthTarget)
    {
        CachedReadOnlyDepthDSV.Reset();
        CachedReadOnlyDepthTarget = DepthTarget;

        if (DepthTarget)
        {
            const FRHIDepthStencilViewDesc DSVDesc = FRHIDepthStencilViewDesc::CreateTexture2D(DepthTarget->GetDesc().Format, 0, EDepthStencilViewFlags::ReadOnlyDepth);
            CachedReadOnlyDepthDSV = RHI::CreateDepthStencilView(DepthTarget, DSVDesc);
        }
    }

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(CachedReadOnlyDepthDSV.Get(), EAttachmentLoadAction::Load);
    CommandList.BeginRenderPass(RenderPassDesc);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const bool bBindless = RHI::bSupportsBindless && GForwardPassBindless && FrameResources.MaterialDataBufferSRV.IsValid();

    struct FForwardPassConstants
    {
        // 0-16
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 NumLightProbes;

        // 16-32
        int32 bEnablePointLightShadows;
        float IndirectSpecularStrength;
        float SpecularAAStrength;
        float SpecularAAMaxRoughnessGain;

        // 32-48
        int32  bEnableSunShadows;
        float  ShadowFilterSize;
        float  ShadowMaxFilterSize;
        uint32 ShadowMapSize;

        // 48-64
        uint32 ShadowNumSamples;
    } ForwardConstants;

    const FSceneDirectionalLight* DirectionalLight = Scene ? Scene->GetDirectionalLight() : nullptr;
    const bool bEnableSunShadows = GShadowsEnabled && GSunShadowsEnabled && DirectionalLight && DirectionalLight->bCastShadows && FrameResources.ShadowCascades;

    const FDirectionalShadowSettingsHLSL ShadowSettings = FShadowMaskRenderPass::CreateShadowSettings(FrameResources, GetRenderer()->GetFrameCounter().GetFrameIndex());

    ForwardConstants.NumPointLights              = FrameResources.PointLightsData.Size();
    ForwardConstants.NumShadowCastingPointLights = FrameResources.ShadowCastingPointLightsData.Size();
    ForwardConstants.NumSkyLightMips             = 0;
    ForwardConstants.NumLightProbes              = FrameResources.LightProbeInfos.Size();
    ForwardConstants.bEnablePointLightShadows    = (GPointLightShadowsEnabled && GShadowsEnabled) ? 1 : 0;
    ForwardConstants.IndirectSpecularStrength    = GIndirectSpecularStrength;
    ForwardConstants.SpecularAAStrength          = GBasePassSpecularAAStrength;
    ForwardConstants.SpecularAAMaxRoughnessGain  = GBasePassSpecularAAMaxRoughnessGain;
    ForwardConstants.bEnableSunShadows           = bEnableSunShadows ? 1 : 0;
    ForwardConstants.ShadowFilterSize            = ShadowSettings.FilterSize;
    ForwardConstants.ShadowMaxFilterSize         = ShadowSettings.MaxFilterSize;
    ForwardConstants.ShadowMapSize               = ShadowSettings.ShadowMapSize;
    ForwardConstants.ShadowNumSamples            = ShadowSettings.NumSamples;

    FRHIShaderResourceView* SkyboxSRV        = nullptr;
    FRHIShaderResourceView* ProbeDiffuseSRV  = nullptr;
    FRHIShaderResourceView* ProbeSpecularSRV = nullptr;

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
        {
            ForwardConstants.NumSkyLightMips = SkyLight->SpecularCubeMap->GetDesc().NumMipLevels;
        }

        if (FSceneSkybox* Skybox = Scene->GetSkybox())
        {
            SkyboxSRV = Skybox->CubeMap->GetShaderResourceView();
        }

        if (!Scene->GetLightProbes().IsEmpty())
        {
            // TODO: Support more than the first probe, as in the deferred light pass
            if (FSceneLightProbe* LightProbe = Scene->GetLightProbes().First())
            {
                ProbeDiffuseSRV  = LightProbe->DiffuseCubeMap->GetShaderResourceView();
                ProbeSpecularSRV = LightProbe->SpecularCubeMap->GetShaderResourceView();
            }
        }
    }

    const auto BindFrameResources = [&](FRHIVertexShader* VShader, FRHIPixelShader* PShader)
    {
        // The vertex shader projects with CameraBuffer.ViewProjection, so it needs its own binding
        CommandList.SetConstantBuffer(VShader, FrameResources.CameraBuffer.Get(), 0);

        constexpr uint32 NumConstants = sizeof(FForwardPassConstants) / sizeof(uint32);
        CommandList.SetShaderConstants(PShader, &ForwardConstants, NumConstants);

        CommandList.SetConstantBuffer(PShader, FrameResources.CameraBuffer.Get(), 0);
        CommandList.SetConstantBuffer(PShader, FrameResources.PointLightsBuffer.Get(), 1);
        CommandList.SetConstantBuffer(PShader, FrameResources.PointLightsPosRadBuffer.Get(), 2);
        CommandList.SetConstantBuffer(PShader, FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
        CommandList.SetConstantBuffer(PShader, FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
        CommandList.SetConstantBuffer(PShader, FrameResources.DirectionalLightDataBuffer.Get(), 5);
        CommandList.SetConstantBuffer(PShader, FrameResources.LightProbeBuffer.Get(), 7);

        if (Scene)
        {
            if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
            {
                CommandList.SetShaderResourceView(PShader, SkyLight->DiffuseCubeMap->GetShaderResourceView(), 0);
                CommandList.SetShaderResourceView(PShader, SkyLight->SpecularCubeMap->GetShaderResourceView(), 1);
            }
        }

        CommandList.SetShaderResourceView(PShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
        CommandList.SetShaderResourceView(PShader, FrameResources.ShadowCascades->GetShaderResourceView(), 3);
        CommandList.SetShaderResourceView(PShader, FrameResources.PointLightShadowMaps->GetShaderResourceView(), 4);
        CommandList.SetShaderResourceView(PShader, SkyboxSRV, 13);
        CommandList.SetShaderResourceView(PShader, ProbeDiffuseSRV, 14);
        CommandList.SetShaderResourceView(PShader, ProbeSpecularSRV, 15);
        CommandList.SetShaderResourceView(PShader, FrameResources.CascadeSplitsBufferSRV.Get(), 16);

        CommandList.SetSamplerState(PShader, FrameResources.IntegrationLUTSampler.Get(), 1);
        CommandList.SetSamplerState(PShader, FrameResources.LightProbeSampler.Get(), 2);
        CommandList.SetSamplerState(PShader, FrameResources.PointLightShadowSampler.Get(), 3);
        CommandList.SetSamplerState(PShader, FrameResources.ShadowSamplerPointCmp.Get(), 4);
        CommandList.SetSamplerState(PShader, FrameResources.ShadowSamplerLinearCmp.Get(), 5);
        CommandList.SetSamplerState(PShader, FrameResources.ShadowSamplerPoint.Get(), 6);
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

        const FMaterialFeatures    Features(Batch.EffectiveMaterialFlags);
        const FGraphicsPipelineKey PSOKey = CreateForwardPassPSOKey(Features, bBindless, Batch.Declaration);

        FGraphicsPipelineStateInstance* MaterialPipeline = PipelineStates.Find(PSOKey);
        if (!MaterialPipeline)
        {
            MaterialPipeline = CompilePipelineState(Features, bBindless, Batch.Declaration);
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
            BindFrameResources(VShader.Get(), PShader.Get());
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

            const int32 MaxMaterialIndex = Math::Max<int32>(int32(FrameResources.MaterialData.Size()) - 1, 0);
            StaticMesh->PerObjectBuffer.MaterialIndex = uint32(Math::Clamp<int32>(Material->GetBufferIndex(), 0, MaxMaterialIndex));
            CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);

            CommandList.SetConstantBuffer(VShader.Get(), FrameResources.PerObjectBuffer.Get(), 6);
            CommandList.SetConstantBuffer(PShader.Get(), FrameResources.PerObjectBuffer.Get(), 6);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();
}
