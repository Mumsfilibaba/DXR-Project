#include "Renderer/SceneRenderer.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Settings/ShadowSettings.h"
#include "Renderer/Settings/ReflectionSettings.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RHI/RHICore.h"

#if EDITOR_BUILD
    #include "Renderer/Passes/Editor/EditorNoJitterDepthPass.h"
    #include "Renderer/Passes/Editor/EditorSelectionIDPass.h"
    #include "Renderer/Passes/Editor/FinalCompositePass.h"
    #include "Renderer/Passes/Editor/SelectionOutlinePass.h"
#endif

static FRenderGraphTextureDesc CreateGBufferDesc(EFormat Format, uint32 Width, uint32 Height)
{
    return FRenderGraphTextureDesc::CreateTexture2D(Format, Width, Height, 1, 1,
        ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture);
}

static FRenderGraphTextureDesc CreateDepthDesc(uint32 Width, uint32 Height)
{
    ETextureUsageFlags Usage = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture;
    if (RHI::bSupportsProgrammableSamplePositions)
    {
        Usage |= ETextureUsageFlags::SamplePositionsCompatible;
    }

    return FRenderGraphTextureDesc::CreateTexture2D(RendererTextureFormats::DepthBufferFormat, Width, Height, 1, 1, Usage,
        FClearValue(RendererTextureFormats::DepthBufferFormat, 1.0f, 0));
}

static FRenderGraphTextureDesc CreateUnorderedAccessDesc(EFormat Format, uint32 Width, uint32 Height)
{
    return FRenderGraphTextureDesc::CreateTexture2D(Format, Width, Height, 1, 1,
        ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture);
}

static FRenderGraphTextureDesc CreateSceneTargetDesc(EFormat Format, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget
            | ETextureUsageFlags::UnorderedAccessTexture
            | ETextureUsageFlags::ShaderResourceTexture
            | ETextureUsageFlags::CopySource;
            
    return FRenderGraphTextureDesc::CreateTexture2D(Format, Width, Height, 1, 1, Usage);
}

static FRenderGraphTextureDesc CreateReducedDepthDesc(uint32 Width, uint32 Height)
{
    constexpr uint32 Alignment = 16;
    const uint32 ReducedWidth  = Math::DivideByMultiple(Width, Alignment);
    const uint32 ReducedHeight = Math::DivideByMultiple(Height, Alignment);

    return FRenderGraphTextureDesc::CreateTexture2D(EFormat::R32G32_Float, ReducedWidth, ReducedHeight, 1, 1,
        ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture);
}

static void CreateSceneGraphViews(FRenderGraphBuilder& GraphBuilder, FSceneRenderGraphContext& Context, const FFrameResources& FrameResources)
{
    if (Context.GBufferDepth)
    {
        Context.GBufferDepthReadOnlyDSV = GraphBuilder.CreateDSV(Context.GBufferDepth,
            FRHIDepthStencilViewDesc::CreateTexture2D(RendererTextureFormats::DepthBufferFormat, 0, EDepthStencilViewFlags::ReadOnlyDepth),
            "GBufferDepthReadOnlyDSV");
    }

    if (Context.SceneTarget)
    {
        Context.SceneTargetRenderTargetView = GraphBuilder.CreateRTV(Context.SceneTarget,
            FRHIRenderTargetViewDesc::CreateTexture2D(RendererTextureFormats::SceneTargetFormat, 0), "SceneTargetRenderTargetView");
    }

    if (Context.CascadeMatrixBuffer)
    {
        Context.CascadeMatrixBufferUAV = GraphBuilder.CreateUAV(Context.CascadeMatrixBuffer,
            FRHIUnorderedAccessViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES), "CascadeMatrixBufferUAV");
        Context.CascadeMatrixBufferSRV = GraphBuilder.CreateSRV(Context.CascadeMatrixBuffer,
            FRHIShaderResourceViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES), "CascadeMatrixBufferSRV");
    }

    if (Context.CascadeSplitsBuffer)
    {
        Context.CascadeSplitsBufferUAV = GraphBuilder.CreateUAV(Context.CascadeSplitsBuffer,
            FRHIUnorderedAccessViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES), "CascadeSplitsBufferUAV");
        Context.CascadeSplitsBufferSRV = GraphBuilder.CreateSRV(Context.CascadeSplitsBuffer,
            FRHIShaderResourceViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES), "CascadeSplitsBufferSRV");
    }

    if (Context.ShadowCascades)
    {
        const EFormat ShadowMapFormat = Context.ShadowCascades->GetDesc().TextureDesc.Format;

        Context.ShadowCascadesCombinedDSV = GraphBuilder.CreateDSV(Context.ShadowCascades,
            FRHIDepthStencilViewDesc::CreateTexture2DArray(ShadowMapFormat, 0, 0, NUM_SHADOW_CASCADES),
            "ShadowCascadesCombinedDSV");

        for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
        {
            Context.ShadowCascadeDSVs[CascadeIndex] = GraphBuilder.CreateDSV(Context.ShadowCascades,
                FRHIDepthStencilViewDesc::CreateTexture2DArray(ShadowMapFormat, 0, CascadeIndex, 1),
                "ShadowCascadeDSV");
        }
    }

    if (Context.PointLightShadowMaps)
    {
        const EFormat ShadowMapFormat = Context.PointLightShadowMaps->GetDesc().TextureDesc.Format;

        Context.PointLightShadowMapDSVs.Clear();
        Context.PointLightShadowMapFaceDSVs.Clear();
        Context.PointLightShadowMapDSVs.Reserve(FrameResources.MaxPointLightShadows);
        Context.PointLightShadowMapFaceDSVs.Reserve(FrameResources.MaxPointLightShadows * RHI_NUM_CUBE_FACES);

        for (uint32 LightIndex = 0; LightIndex < FrameResources.MaxPointLightShadows; ++LightIndex)
        {
            Context.PointLightShadowMapDSVs.Add(GraphBuilder.CreateDSV(Context.PointLightShadowMaps,
                FRHIDepthStencilViewDesc::CreateTextureCubeArray(ShadowMapFormat, 0, static_cast<uint16>(LightIndex), 1),
                "PointLightShadowMapDSV"));

            for (uint32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; ++FaceIndex)
            {
                Context.PointLightShadowMapFaceDSVs.Add(GraphBuilder.CreateDSV(Context.PointLightShadowMaps,
                    FRHIDepthStencilViewDesc::CreateTexture2DArray(ShadowMapFormat, 0,
                        static_cast<uint16>((LightIndex * RHI_NUM_CUBE_FACES) + FaceIndex), 1), "PointLightShadowMapFaceDSV"));
            }
        }
    }
}

void FSceneRenderer::BuildAndExecuteSceneGraph(const FSceneRenderView& SceneRenderView, FScene* Scene, const TArray<uint32>& SelectedObjectIDs)
{
    FFrameResources& FrameResourcesRef = Resources;
    const uint32     RenderWidth       = FrameResourcesRef.CurrentRenderWidth;
    const uint32     RenderHeight      = FrameResourcesRef.CurrentRenderHeight;

    FRenderGraphBuilder GraphBuilder("Scene");

    FSceneRenderGraphContext Context;
    Context.SceneRenderer         = this;
    Context.Scene                 = Scene;
    Context.View                  = &SceneRenderView;
    Context.FrameResources        = &FrameResourcesRef;
    Context.SelectedObjectIDs     = &SelectedObjectIDs;
    Context.GBufferAlbedo         = GraphBuilder.CreateTexture(CreateGBufferDesc(RendererTextureFormats::AlbedoFormat, RenderWidth, RenderHeight), "GBufferAlbedo");
    Context.GBufferNormal         = GraphBuilder.CreateTexture(CreateGBufferDesc(RendererTextureFormats::NormalFormat, RenderWidth, RenderHeight), "GBufferNormal");
    Context.GBufferMaterial       = GraphBuilder.CreateTexture(CreateGBufferDesc(RendererTextureFormats::MaterialFormat, RenderWidth, RenderHeight), "GBufferMaterial");
    Context.GBufferVelocity       = GraphBuilder.CreateTexture(CreateGBufferDesc(RendererTextureFormats::VelocityFormat, RenderWidth, RenderHeight), "GBufferVelocity");
    Context.GBufferDepth          = GraphBuilder.CreateTexture(CreateDepthDesc(RenderWidth, RenderHeight), "SceneDepth");
    Context.SSAOBuffer            = GraphBuilder.CreateTexture(CreateUnorderedAccessDesc(RendererTextureFormats::SSAOBufferFormat, RenderWidth, RenderHeight), "SSAO");
    Context.SceneTarget           = GraphBuilder.CreateTexture(CreateSceneTargetDesc(RendererTextureFormats::SceneTargetFormat, RenderWidth, RenderHeight), "SceneTarget");
    Context.DirectionalShadowMask = GraphBuilder.CreateTexture(CreateUnorderedAccessDesc(RendererTextureFormats::ShadowMaskFormat, RenderWidth, RenderHeight), "DirectionalShadowMask");
    Context.CascadeIndexBuffer    = GraphBuilder.CreateTexture(CreateUnorderedAccessDesc(EFormat::R32_Float, RenderWidth, RenderHeight), "CascadeIndexBuffer");
    Context.ReducedDepthBuffer0   = GraphBuilder.CreateTexture(CreateReducedDepthDesc(RenderWidth, RenderHeight), "ReducedDepth0");
    Context.ReducedDepthBuffer1   = GraphBuilder.CreateTexture(CreateReducedDepthDesc(RenderWidth, RenderHeight), "ReducedDepth1");

    const bool bIsRayTracingActive   = RHI::bSupportsRayTracing && GRayTracingEnabled;
    const bool bNeedsPrimaryRayDebug = bIsRayTracingActive && RayTracingPrimaryDebugPass->IsEnabled(SceneRenderView);

    if (bIsRayTracingActive)
    {
        if (!bRayTracingWasActive)
        {
            ReflectionDenoisePass->InvalidateHistory();
        }

        RayTracingSceneBuilder->BuildSceneAccelerationData(CommandList, FrameResourcesRef, Scene,
            RayTracingReflectionsPass->NeedsBindlessData() || bNeedsPrimaryRayDebug);
    }
    else if (bRayTracingWasActive)
    {
        RayTracingSceneBuilder->ReleaseRayTracingResources(Scene);
    }

    bRayTracingWasActive = bIsRayTracingActive;

#if EDITOR_BUILD
    Context.TonemappedTarget = GraphBuilder.CreateTexture(
        CreateGBufferDesc(RendererTextureFormats::RenderTargetFormat, RenderWidth, RenderHeight), "TonemappedTarget");

    if (FrameResourcesRef.EditorNoJitterDepth)
    {
        Context.EditorNoJitterDepth = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.EditorNoJitterDepth.Get(), "EditorNoJitterDepth",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FrameResourcesRef.EditorObjectID_NoJitter)
    {
        Context.EditorObjectID_NoJitter = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.EditorObjectID_NoJitter.Get(), "EditorObjectID",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    // The outline chain ping-pongs between these, so the graph has to see every step of it
    if (FRHITexture* SelectionMask = SelectionOutlinePass->GetSelectionMask())
    {
        Context.SelectionMask = GraphBuilder.RegisterExternalTexture(SelectionMask, "SelectionMask",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FRHITexture* ErosionTemp = SelectionOutlinePass->GetErosionTemp())
    {
        Context.SelectionErosionTemp = GraphBuilder.RegisterExternalTexture(ErosionTemp, "SelectionErosionTemp",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FRHITexture* ErodedMask = SelectionOutlinePass->GetErodedMask())
    {
        Context.SelectionErodedMask = GraphBuilder.RegisterExternalTexture(ErodedMask, "SelectionErodedMask",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FRHITexture* DilationTemp = SelectionOutlinePass->GetDilationTemp())
    {
        Context.SelectionDilationTemp = GraphBuilder.RegisterExternalTexture(DilationTemp, "SelectionDilationTemp",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FRHITexture* DilatedMask = SelectionOutlinePass->GetDilatedMask())
    {
        Context.SelectionDilatedMask = GraphBuilder.RegisterExternalTexture(DilatedMask, "SelectionDilatedMask",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FRHITexture* RingMask = SelectionOutlinePass->GetRingMask())
    {
        Context.SelectionRing = GraphBuilder.RegisterExternalTexture(RingMask, "SelectionRing",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }

    if (FRHIBuffer* SelectedIDsBuffer = SelectionOutlinePass->GetSelectedIDsBuffer())
    {
        Context.SelectedIDsBuffer = GraphBuilder.RegisterExternalBuffer(SelectedIDsBuffer, "SelectedIDs",
            ERHIResourceState::PixelShaderResource, ERHIResourceState::PixelShaderResource);
    }
#endif

    if (SceneRenderView.RenderTarget)
    {
        Context.BackBuffer = GraphBuilder.RegisterExternalTexture(SceneRenderView.RenderTarget, "BackBuffer",
            ERHIResourceState::RenderTarget, ERHIResourceState::RenderTarget);
    }

    Context.CameraBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.CameraBuffer.Get(), "Camera",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.PerObjectBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.PerObjectBuffer.Get(), "PerObject",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.MaterialDataBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.MaterialDataBuffer.Get(), "MaterialData",
        ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);

    Context.PointLightsBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.PointLightsBuffer.Get(), "PointLights",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.PointLightsPosRadBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.PointLightsPosRadBuffer.Get(), "PointLightsPosRad",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.ShadowCastingPointLightsBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.ShadowCastingPointLightsBuffer.Get(), "ShadowCastingPointLights",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.ShadowCastingPointLightsPosRadBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.ShadowCastingPointLightsPosRadBuffer.Get(), "ShadowCastingPointLightsPosRad",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.DirectionalLightDataBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.DirectionalLightDataBuffer.Get(), "DirectionalLight",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.CascadeGenerationDataBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.CascadeGenerationDataBuffer.Get(), "CascadeGeneration",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    Context.CascadeMatrixBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.CascadeMatrixBuffer.Get(), "CascadeMatrices",
        ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);

    Context.CascadeSplitsBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.CascadeSplitsBuffer.Get(), "CascadeSplits",
        ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);

    Context.LightProbeBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.LightProbeBuffer.Get(), "LightProbes",
        ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);

    {
        auto ClampAndSnapPow2 = [](int32 MinSize, int32 MaxSize, int32 Value) -> int32
        {
            return Math::ClosestPowerOfTwo(Math::Clamp(Value, MinSize, MaxSize));
        };

        const int32 NewCascadeSize = ClampAndSnapPow2(512, 4096, GCSMCascadeSize);
        if (NewCascadeSize != FrameResourcesRef.CascadeSize)
        {
            FrameResourcesRef.CascadeSize = NewCascadeSize;
            CascadedShadowsRenderPass->CreateResources(FrameResourcesRef);
        }

        const int32 NewPointLightSize = ClampAndSnapPow2(128, 1024, GPointLightShadowMapSize);
        if (NewPointLightSize != FrameResourcesRef.PointLightShadowSize)
        {
            FrameResourcesRef.PointLightShadowSize = NewPointLightSize;
            PointLightRenderPass->CreateResources(FrameResourcesRef);
        }
    }

    if (FrameResourcesRef.ShadowCascades)
    {
        Context.ShadowCascades = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.ShadowCascades.Get(), "ShadowCascades",
            ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);
    }

    if (FrameResourcesRef.PointLightShadowMaps)
    {
        Context.PointLightShadowMaps = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.PointLightShadowMaps.Get(), "PointLightShadowMaps",
            ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);
    }

    if (FRHIBuffer* PerCascadeBuffer = CascadedShadowsRenderPass->GetPerCascadeBuffer())
    {
        Context.PerCascadeBuffer = GraphBuilder.RegisterExternalBuffer(PerCascadeBuffer, "PerCascade",
            ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);
    }

    if (FRHIBuffer* ShadowSettingsBuffer = ShadowMaskRenderPass->GetShadowSettingsBuffer())
    {
        Context.ShadowSettingsBuffer = GraphBuilder.RegisterExternalBuffer(ShadowSettingsBuffer, "ShadowSettings",
            ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);
    }

    if (FRHIBuffer* PerShadowMapBuffer = PointLightRenderPass->GetPerShadowMapBuffer())
    {
        Context.PerShadowMapBuffer = GraphBuilder.RegisterExternalBuffer(PerShadowMapBuffer, "PerShadowMap",
            ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);
    }

    if (FRHIBuffer* SinglePassShadowMapBuffer = PointLightRenderPass->GetSinglePassShadowMapBuffer())
    {
        Context.SinglePassShadowMapBuffer = GraphBuilder.RegisterExternalBuffer(SinglePassShadowMapBuffer, "SinglePassShadowMap",
            ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);
    }

    if (FrameResourcesRef.IntegrationLUT)
    {
        Context.IntegrationLUT = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.IntegrationLUT.Get(), "IntegrationLUT",
            ERHIResourceState::ShaderResource, ERHIResourceState::ShaderResource);
    }

    if (FRHIBuffer* SkyboxVertexBuffer = SkyboxRenderPass->GetVertexBuffer())
    {
        Context.SkyboxVertexBuffer = GraphBuilder.RegisterExternalBuffer(SkyboxVertexBuffer, "SkyboxVertices",
            ERHIResourceState::VertexBuffer, ERHIResourceState::VertexBuffer);
    }

    if (FRHIBuffer* SkyboxIndexBuffer = SkyboxRenderPass->GetIndexBuffer())
    {
        Context.SkyboxIndexBuffer = GraphBuilder.RegisterExternalBuffer(SkyboxIndexBuffer, "SkyboxIndices",
            ERHIResourceState::IndexBuffer, ERHIResourceState::IndexBuffer);
    }

    if (FrameResourcesRef.RayTracingOutput)
    {
        Context.RayTracingOutput = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.RayTracingOutput.Get(), "RayTracingOutput",
            ERHIResourceState::UnorderedAccess, ERHIResourceState::UnorderedAccess);
    }

    if (FrameResourcesRef.ReflectionTrace)
    {
        Context.ReflectionTrace = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.ReflectionTrace.Get(), "ReflectionTrace",
            ERHIResourceState::UnorderedAccess, ERHIResourceState::UnorderedAccess);
    }

    const CHAR* ReflectionHistoryNames[2] =
    {
        "ReflectionHistory0", "ReflectionHistory1"
    };

    const CHAR* ReflectionMomentsNames[2] =
    {
        "ReflectionMoments0", "ReflectionMoments1"
    };

    const CHAR* ReflectionDenoisedNames[2] =
    {
        "ReflectionDenoised0", "ReflectionDenoised1"
    };

    for (int32 ReflectionIndex = 0; ReflectionIndex < 2; ++ReflectionIndex)
    {
        if (FrameResourcesRef.ReflectionHistory[ReflectionIndex])
        {
            Context.ReflectionHistory[ReflectionIndex] = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.ReflectionHistory[ReflectionIndex].Get(), ReflectionHistoryNames[ReflectionIndex],
                ERHIResourceState::UnorderedAccess, ERHIResourceState::UnorderedAccess);
        }

        if (FrameResourcesRef.ReflectionMoments[ReflectionIndex])
        {
            Context.ReflectionMoments[ReflectionIndex] = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.ReflectionMoments[ReflectionIndex].Get(), ReflectionMomentsNames[ReflectionIndex],
                ERHIResourceState::UnorderedAccess, ERHIResourceState::UnorderedAccess);
        }

        if (FrameResourcesRef.ReflectionDenoised[ReflectionIndex])
        {
            Context.ReflectionDenoised[ReflectionIndex] = GraphBuilder.RegisterExternalTexture(FrameResourcesRef.ReflectionDenoised[ReflectionIndex].Get(), ReflectionDenoisedNames[ReflectionIndex],
                ERHIResourceState::UnorderedAccess, ERHIResourceState::UnorderedAccess);
        }
    }

    if (FrameResourcesRef.RayTracingSceneConstantsBuffer)
    {
        Context.RayTracingSceneConstantsBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.RayTracingSceneConstantsBuffer.Get(), "RayTracingSceneConstants",
            ERHIResourceState::ConstantBuffer, ERHIResourceState::ConstantBuffer);
    }

    if (FrameResourcesRef.RayTracingGeometryTableBuffer)
    {
        Context.RayTracingGeometryTableBuffer = GraphBuilder.RegisterExternalBuffer(FrameResourcesRef.RayTracingGeometryTableBuffer.Get(), "RayTracingGeometryTable",
            ERHIResourceState::GenericRead, ERHIResourceState::GenericRead);
    }

    if (FRHITexture* ReflectionNoise = RayTracingReflectionsPass->GetReflectionNoiseMask())
    {
        Context.ReflectionNoise = GraphBuilder.RegisterExternalTexture(ReflectionNoise, "ReflectionNoise",
            ERHIResourceState::ShaderResource, ERHIResourceState::ShaderResource);
    }

    if (FRHITexture* HistoryBuffer = TemporalAntiAliasing->GetHistoryBuffer(0))
    {
        Context.TAAHistory[0] = GraphBuilder.RegisterExternalTexture(HistoryBuffer, "TAAHistory0",
            ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);
    }

    if (FRHITexture* HistoryBuffer = TemporalAntiAliasing->GetHistoryBuffer(1))
    {
        Context.TAAHistory[1] = GraphBuilder.RegisterExternalTexture(HistoryBuffer, "TAAHistory1",
            ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);
    }

    CreateSceneGraphViews(GraphBuilder, Context, FrameResourcesRef);

    const bool bRestorePrevSamplePositions = PrevFrameSamplePositions.NumSamplesPerPixel > 0;
    if (bRestorePrevSamplePositions)
    {
        CommandList.SetSamplePositions(PrevFrameSamplePositions);
    }

    DepthPrePass->AddRenderGraphPass(GraphBuilder, Context);
    BasePass->AddRenderGraphPass(GraphBuilder, Context);
    DepthReducePass->AddRenderGraphPass(GraphBuilder, Context);

    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FRHISamplePositionsDesc());
    }

    RayTracingReflectionsPass->AddRenderGraphPass(GraphBuilder, Context, ReflectionDenoisePass->IsDenoiseEnabled(FrameResourcesRef));
    ReflectionDenoisePass->AddRenderGraphPass(GraphBuilder, Context, RayTracingReflectionsPass->IsTraceEnabled());

    ScreenSpaceOcclusionPass->AddRenderGraphPass(GraphBuilder, Context);

    PointLightRenderPass->AddRenderGraphPass(GraphBuilder, Context);
    CascadeGenerationPass->AddRenderGraphPass(GraphBuilder, Context);
    CascadedShadowsRenderPass->AddRenderGraphPass(GraphBuilder, Context);
    ShadowMaskRenderPass->AddRenderGraphPass(GraphBuilder, Context);

    TiledLightPass->AddRenderGraphPass(GraphBuilder, Context);

    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FrameSamplePositions);
    }

    SkyboxRenderPass->AddRenderGraphPass(GraphBuilder, Context);
    ForwardPass->AddRenderGraphPass(GraphBuilder, Context);
    TemporalAntiAliasing->AddRenderGraphPass(GraphBuilder, Context);

    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FRHISamplePositionsDesc());
    }

#if EDITOR_BUILD
    EditorNoJitterDepthPass->AddRenderGraphPass(GraphBuilder, Context);
    EditorSelectionIDPass->AddRenderGraphPass(GraphBuilder, Context);
    SelectionOutlinePass->AddRenderGraphPass(GraphBuilder, Context);
#endif

    FXAAPass->AddRenderGraphPass(GraphBuilder, Context);

#if EDITOR_BUILD
    TonemapPass->AddRenderGraphPass(GraphBuilder, Context, nullptr, false);
    FinalCompositePass->AddRenderGraphPass(GraphBuilder, Context);
#else
    TonemapPass->AddRenderGraphPass(GraphBuilder, Context, SceneRenderView.RenderTarget, true);
#endif

    RayTracingPrimaryDebugPass->AddRenderGraphPass(GraphBuilder, Context);

    DebugViewPass->AddRenderGraphPass(GraphBuilder, Context);
    DebugViewPass->AddRenderGraphOverlayPass(GraphBuilder, Context);

    PrevFrameSamplePositions = bUseHardwareJitter ? FrameSamplePositions : FRHISamplePositionsDesc();

    GraphBuilder.Execute(CommandList);

#if EDITOR_BUILD
    RenderThread_ProcessEditorObjectPickRequests(CommandList, FrameResourcesRef, Scene);
#endif
}
