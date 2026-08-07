#include "Core/Containers/Set.h"
#include "Core/Math/Frustum.h"
#include "Core/Templates/Utility/BitCast.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Time/Timespan.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Tasks/Tasks.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/Engine.h"
#if EDITOR_BUILD
    #include "Engine/EditorEngine.h"
#endif
#include "Engine/Resources/Model.h"
#include "Renderer/MaterialBindless.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/EditorSelectionRendering.h"
#include "Renderer/RenderFeatureSettings.h"
#include "Renderer/ShadowSettings.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/RenderSettings.h"

#define SUPPORT_VARIABLE_RATE_SHADING (0)

static bool GEnableSSAO = true;
static FAutoConsoleVariableRef CVarEnableSSAO(
    "Renderer.Feature.SSAO",
    "Enables Screen-Space Ambient Occlusion",
    GEnableSSAO,
    EConsoleVariableFlags::Default);

static bool GEnableFXAA = false;
static FAutoConsoleVariableRef CVarEnableFXAA(
    "Renderer.Feature.FXAA",
    "Enables FXAA for Anti-Aliasing",
    GEnableFXAA,
    EConsoleVariableFlags::Default);

static bool GEnableTemporalAA = true;
static FAutoConsoleVariableRef CVarEnableTemporalAA(
    "Renderer.Feature.TemporalAA",
    "Enables Temporal Anti-Aliasing",
    GEnableTemporalAA,
    EConsoleVariableFlags::Default);

static bool GTemporalAAHardwareJitter = false;
static FAutoConsoleVariableRef CVarTemporalAAHardwareJitter(
    "Renderer.TemporalAA.HardwareJitter",
    "Applies TAA sub-pixel jitter via programmable sample positions instead of a projection-matrix offset",
    GTemporalAAHardwareJitter,
    EConsoleVariableFlags::Default);

static bool GEnableVariableRateShading = false;
static FAutoConsoleVariableRef CVarEnableVariableRateShading(
    "Renderer.Feature.VariableRateShading",
    "Enables VRS (Variable Rate Shading)",
    GEnableVariableRateShading,
    EConsoleVariableFlags::Default);

static bool GPrePassEnabled = true;
static FAutoConsoleVariableRef CVarPrePassEnabled(
    "Renderer.Feature.PrePass",
    "Enables Pre-Pass",
    GPrePassEnabled,
    EConsoleVariableFlags::Default);

#if EDITOR_BUILD
static int32 GEditorPickSearchRadius = 1;
static FAutoConsoleVariableRef CVarEditorPickSearchRadius(
    "Editor.Pick.SearchRadius",
    "When the center pixel returns ObjectID=0, search within this radius (in pixels) for a non-zero ObjectID.",
    GEditorPickSearchRadius,
    EConsoleVariableFlags::Default);

static bool GEditorPickTryFlipY = false;
static FAutoConsoleVariableRef CVarEditorPickTryFlipY(
    "Editor.Pick.TryFlipY",
    "Debug-only: If you suspect the pick coordinates are vertically inverted, also sample a vertically flipped Y window when the normal sample returns ObjectID=0.",
    GEditorPickTryFlipY,
    EConsoleVariableFlags::Default);

static bool GEditorPickDebug = false;
static FAutoConsoleVariableRef CVarEditorPickDebug(
    "Editor.Pick.Debug",
    "Logs editor picking requests and readback results.",
    GEditorPickDebug,
    EConsoleVariableFlags::Default);

static int32 GEditorPickMaxRectRows = 512;
static FAutoConsoleVariableRef CVarEditorPickMaxRectRows(
    "Editor.Pick.MaxRectRows",
    "Caps how many rows a box-select reads back, stepping over the source rows when the box is taller. A full-viewport box at 4K would otherwise read back 33 MB.",
    GEditorPickMaxRectRows,
    EConsoleVariableFlags::Default);
#endif

static bool GBasePassEnabled = true;
static FAutoConsoleVariableRef CVarBasePassEnabled(
    "Renderer.Feature.BasePass",
    "Enables BasePass (Disabling this disables most rendering)",
    GBasePassEnabled,
    EConsoleVariableFlags::Default);

bool GShadowsEnabled = true;
static FAutoConsoleVariableRef CVarShadowsEnabled(
    "Renderer.Feature.Shadows",
    "Enables Rendering of ShadowMaps",
    GShadowsEnabled,
    EConsoleVariableFlags::Default);

static bool GShadowMaskEnabled = true;
static FAutoConsoleVariableRef CVarShadowMaskEnabled(
    "Renderer.Feature.ShadowMask",
    "Enables Rendering of ShadowMask for SunShadows",
    GShadowMaskEnabled,
    EConsoleVariableFlags::Default);

bool GPointLightShadowsEnabled = true;
static FAutoConsoleVariableRef CVarPointLightShadowsEnabled(
    "Renderer.Feature.PointLightShadows",
    "Enables Rendering of PointLight ShadowMaps",
    GPointLightShadowsEnabled,
    EConsoleVariableFlags::Default);

static bool GSunShadowsEnabled = true;
static FAutoConsoleVariableRef CVarSunShadowsEnabled(
    "Renderer.Feature.SunShadows",
    "Enables Rendering of SunLight/DirectionalLight ShadowMaps",
    GSunShadowsEnabled,
    EConsoleVariableFlags::Default);

static bool GSkyboxEnabled = true;
static FAutoConsoleVariableRef CVarSkyboxEnabled(
    "Renderer.Feature.Skybox",
    "Enables Rendering of the Skybox",
    GSkyboxEnabled,
    EConsoleVariableFlags::Default);

static bool GDrawAABBs = false;
static FAutoConsoleVariableRef CVarDrawAABBs(
    "Renderer.Debug.DrawAABBs",
    "Draws all the objects bounding boxes (AABB)",
    GDrawAABBs,
    EConsoleVariableFlags::Default);

static bool GDrawPointLights = false;
static FAutoConsoleVariableRef CVarDrawPointLights(
    "Renderer.Debug.DrawPointLights",
    "Draws all the point-lights as spheres with the light-color",
    GDrawPointLights,
    EConsoleVariableFlags::Default);

static bool GDrawLightProbes = false;
static FAutoConsoleVariableRef CVarDrawLightProbes(
    "Renderer.Debug.LightProbes",
    "Draws all the light-probes as spheres with the cube-map",
    GDrawLightProbes,
    EConsoleVariableFlags::Default);

static bool GVSyncEnabled = false;
static FAutoConsoleVariableRef CVarVSyncEnabled(
    "Renderer.Feature.VerticalSync",
    "Enables Vertical-Sync",
    GVSyncEnabled,
    EConsoleVariableFlags::Default);

static bool GFrustumCullEnabled = true;
static FAutoConsoleVariableRef CVarFrustumCullEnabled(
    "Renderer.Feature.FrustumCulling",
    "Enables Frustum Culling (CPU) for the main scene and for all shadow frustums",
    GFrustumCullEnabled,
    EConsoleVariableFlags::Default);

bool GRayTracingEnabled = false;
static FAutoConsoleVariableRef CVarRayTracingEnabled(
    "Renderer.Feature.RayTracing",
    "Enables ray-traced reflections. Only takes effect when the hardware reports ray tracing support; otherwise the renderer falls back to image-based lighting.",
    GRayTracingEnabled,
    EConsoleVariableFlags::Default);

bool GCSMTightFrustum = true;
static FAutoConsoleVariableRef CVarCSMTightFrustum(
    "Renderer.CSM.TightFrustum",
    "Set to true to reduce the DepthBuffer to find the Min- and Max Depth in the DepthBuffer to be able to create a tight frustum that fits the scene",
    GCSMTightFrustum,
    EConsoleVariableFlags::Default);

static FAutoConsoleCommand CCmdFreezeRendering(
    "Renderer.FreezeRendering",
    "Freezes the updating of Frustum culling",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        GFreezeRendering = !GFreezeRendering;
    }));

// Hardware capability AND the user toggle must both be set.
static FORCEINLINE bool IsRayTracingActive()
{
    return RHI::bSupportsRayTracing && GRayTracingEnabled;
}

FSceneRenderer::FSceneRenderer()
    : Resources()
    , CameraBuffer()
    , HaltonState()
    , DepthPrePass(nullptr)
    , BasePass(nullptr)
    , DepthReducePass(nullptr)
    , TiledLightPass(nullptr)
    , PointLightRenderPass(nullptr)
    , CascadeGenerationPass(nullptr)
    , CascadedShadowsRenderPass(nullptr)
    , ShadowMaskRenderPass(nullptr)
    , ScreenSpaceOcclusionPass(nullptr)
    , SkyboxRenderPass(nullptr)
    , TemporalAA(nullptr)
#if EDITOR_BUILD
    , SelectionOutlinePass(nullptr)
    , EditorNoJitterDepthPass(nullptr)
    , EditorSelectionIDPass(nullptr)
#endif
    , ForwardPass(nullptr)
    , FXAAPass(nullptr)
    , TonemapPass(nullptr)
#if EDITOR_BUILD
    , FinalCompositePass(nullptr)
#endif
    , LightProbeRenderer(nullptr)
    , DebugRenderer(nullptr)
    , DebugViewPass(nullptr)
    , RayTracer(this)
    , LastFrameFinishedEvent(nullptr)
    , TimestampQueries(nullptr)
    , CommandList()
    , ShadingImage(nullptr)
    , ShadingRatePipeline(nullptr)
    , ShadingRateShader(nullptr)
{
}

FSceneRenderer::~FSceneRenderer()
{
    FRHICommandListExecutor::Get().WaitForGPU();

    CommandList.Reset();

    SAFE_DELETE(DepthPrePass);
    SAFE_DELETE(BasePass);
    SAFE_DELETE(DepthReducePass);
    SAFE_DELETE(TiledLightPass);
    SAFE_DELETE(PointLightRenderPass);
    SAFE_DELETE(CascadeGenerationPass);
    SAFE_DELETE(CascadedShadowsRenderPass);
    SAFE_DELETE(ShadowMaskRenderPass);
    SAFE_DELETE(ScreenSpaceOcclusionPass);
    SAFE_DELETE(SkyboxRenderPass);
    SAFE_DELETE(TemporalAA);
#if EDITOR_BUILD
    SAFE_DELETE(SelectionOutlinePass);
    SAFE_DELETE(EditorNoJitterDepthPass);
    SAFE_DELETE(EditorSelectionIDPass);
#endif
    SAFE_DELETE(ForwardPass);
    SAFE_DELETE(FXAAPass);
    SAFE_DELETE(TonemapPass);
#if EDITOR_BUILD
    SAFE_DELETE(FinalCompositePass);
#endif
    SAFE_DELETE(LightProbeRenderer);
    SAFE_DELETE(DebugRenderer);
    SAFE_DELETE(DebugViewPass);

    RayTracer.Release();

    Resources.Release();

    ShadingImage.Reset();
    ShadingRatePipeline.Reset();
    ShadingRateShader.Reset();

    TimestampQueries.Reset();

}

bool FSceneRenderer::Initialize()
{ 
    Resources.CurrentRenderWidth  = RenderSettings::GetRenderWidth();
    Resources.CurrentRenderHeight = RenderSettings::GetRenderHeight();

    if (RenderSettings::NeedsResize())
    {
        RenderSettings::OnDidChangeRenderResolution(Resources.CurrentRenderWidth, Resources.CurrentRenderHeight);
    }

    FRHIBufferDesc ConstantBufferDesc;
    ConstantBufferDesc.Size   = sizeof(FCameraHLSL);
    ConstantBufferDesc.Stride = sizeof(FCameraHLSL);
    ConstantBufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

    Resources.CameraBuffer = RHI::CreateBuffer(ConstantBufferDesc, ERHIResourceState::Common, nullptr);
    if (!Resources.CameraBuffer)
    {
        LOG_ERROR("[Renderer]: Failed to create CameraBuffer");
        return false;
    }
    else
    {
        Resources.CameraBuffer->SetDebugName("CameraBuffer");
    }

    FRHIBufferDesc PerObjectConstantBufferDesc;
    PerObjectConstantBufferDesc.Size   = sizeof(FPerObjectHLSL);
    PerObjectConstantBufferDesc.Stride = sizeof(FPerObjectHLSL);
    PerObjectConstantBufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Transient;

    Resources.PerObjectBuffer = RHI::CreateBuffer(PerObjectConstantBufferDesc, ERHIResourceState::Common, nullptr);
    if (!Resources.PerObjectBuffer)
    {
        LOG_ERROR("[Renderer]: Failed to create PerObjectBuffer");
        return false;
    }
    else
    {
        Resources.PerObjectBuffer->SetDebugName("PerObjectBuffer");
    }

    {
        FRHISamplerStateDesc SamplerStateDesc;
        SamplerStateDesc.AddressU       = ESamplerMode::Clamp;
        SamplerStateDesc.AddressV       = ESamplerMode::Clamp;
        SamplerStateDesc.AddressW       = ESamplerMode::Clamp;
        SamplerStateDesc.Filter         = ESamplerFilter::MinMagMipPoint;
        SamplerStateDesc.ComparisonFunc = EComparisonFunc::Unknown;
        SamplerStateDesc.MinLOD         = 0.0f;
        SamplerStateDesc.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        Resources.ShadowSamplerPoint = RHI::CreateSamplerState(SamplerStateDesc);
        if (!Resources.ShadowSamplerPoint)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    {
        FRHISamplerStateDesc SamplerStateDesc;
        SamplerStateDesc.AddressU       = ESamplerMode::Clamp;
        SamplerStateDesc.AddressV       = ESamplerMode::Clamp;
        SamplerStateDesc.AddressW       = ESamplerMode::Clamp;
        SamplerStateDesc.Filter         = ESamplerFilter::Comparison_MinMagMipPoint;
        SamplerStateDesc.ComparisonFunc = EComparisonFunc::LessEqual;
        SamplerStateDesc.MinLOD         = 0.0f;
        SamplerStateDesc.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        Resources.ShadowSamplerPointCmp = RHI::CreateSamplerState(SamplerStateDesc);
        if (!Resources.ShadowSamplerPointCmp)
        {
            DEBUG_BREAK();
            return false;
        }

        SamplerStateDesc.Filter = ESamplerFilter::Comparison_MinMagMipLinear;

        Resources.ShadowSamplerLinearCmp = RHI::CreateSamplerState(SamplerStateDesc);
        if (!Resources.ShadowSamplerLinearCmp)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    {
        FRHISamplerStateDesc SamplerStateDesc;
        SamplerStateDesc.AddressU       = ESamplerMode::Wrap;
        SamplerStateDesc.AddressV       = ESamplerMode::Wrap;
        SamplerStateDesc.AddressW       = ESamplerMode::Wrap;
        SamplerStateDesc.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        SamplerStateDesc.ComparisonFunc = EComparisonFunc::LessEqual;
        SamplerStateDesc.MinLOD         = 0.0f;
        SamplerStateDesc.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        Resources.PointLightShadowSampler = RHI::CreateSamplerState(SamplerStateDesc);
        if (!Resources.PointLightShadowSampler)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    if (!InitShadingImage())
    {
        return false;
    }

    if (!Resources.Initialize())
    {
        return false;
    }

    if (!InitializeRenderPasses())
    {
        return false;
    }

    if (RHI::bSupportsRayTracing)
    {
        if (!RayTracer.Initialize(Resources))
        {
            return false;
        }
    }

    return true;
}

bool FSceneRenderer::InitializeRenderPasses()
{
    DebugRenderer = new FDebugRenderer(this);
    if (!DebugRenderer->Initialize(Resources))
    {
        return false;
    }

    DebugViewPass = new FDebugViewPass(this);
    if (!DebugViewPass->Initialize(Resources))
    {
        return false;
    }

    DepthPrePass = new FDepthPrePass(this);
    if (!DepthPrePass->Initialize(Resources))
    {
        return false;
    }

    BasePass = new FDeferredBasePass(this);
    if (!BasePass->Initialize(Resources))
    {
        return false;
    }

    TiledLightPass = new FTiledLightPass(this);
    if (!TiledLightPass->Initialize(Resources))
    {
        return false;
    }

    DepthReducePass = new FDepthReducePass(this);
    if (!DepthReducePass->Initialize(Resources))
    {
        return false;
    }

    PointLightRenderPass = new FPointLightRenderPass(this);
    if (!PointLightRenderPass->Initialize(Resources))
    {
        return false;
    }

    CascadeGenerationPass = new FCascadeGenerationPass(this);
    if (!CascadeGenerationPass->Initialize(Resources))
    {
        return false;
    }

    CascadedShadowsRenderPass = new FCascadedShadowsRenderPass(this);
    if (!CascadedShadowsRenderPass->Initialize(Resources))
    {
        return false;
    }

    ShadowMaskRenderPass = new FShadowMaskRenderPass(this);
    if (!ShadowMaskRenderPass->Initialize(Resources))
    {
        return false;
    }

    ScreenSpaceOcclusionPass = new FScreenSpaceOcclusionPass(this);
    if (!ScreenSpaceOcclusionPass->Initialize(Resources))
    {
        return false;
    }

    SkyboxRenderPass = new FSkyboxRenderPass(this);
    if (!SkyboxRenderPass->Initialize(Resources))
    {
        return false;
    }

    TemporalAA = new FTemporalAA(this);
    if (!TemporalAA->Initialize(Resources))
    {
        return false;
    }

#if EDITOR_BUILD
    SelectionOutlinePass = new FSelectionOutlinePass(this);
    if (!SelectionOutlinePass->Initialize(Resources))
    {
        return false;
    }

    EditorNoJitterDepthPass = new FEditorNoJitterDepthPass(this);
    if (!EditorNoJitterDepthPass->Initialize(Resources))
    {
        return false;
    }

    EditorSelectionIDPass = new FEditorSelectionIDPass(this);
    if (!EditorSelectionIDPass->Initialize(Resources))
    {
        return false;
    }
#endif

    ForwardPass = new FForwardPass(this);
    if (!ForwardPass->Initialize(Resources))
    {
        return false;
    }

    TonemapPass = new FTonemapPass(this);
    if (!TonemapPass->Initialize(Resources))
    {
        return false;
    }

#if EDITOR_BUILD
    FinalCompositePass = new FFinalCompositePass(this);
    if (!FinalCompositePass->Initialize(Resources))
    {
        return false;
    }
#endif

    FXAAPass = new FFXAAPass(this);
    if (!FXAAPass->Initialize(Resources))
    {
        return false;
    }

    LightProbeRenderer = new FLightProbeRenderer(this);
    if (!LightProbeRenderer->Initialize(Resources))
    {
        return false;
    }

    return true;
}

void FSceneRenderer::RenderThread_BeginSceneCommandList(const FSceneRenderPacket& Packet)
{
    CHECK_RENDER_THREAD();

    FRHICommandListExecutor::Get().Tick();

    // Update FrameCounter
    FrameCounter.NextFrame();

    CommandList.BeginFrame();
    CommandList.PushEvent("Frame");

    {
        TRACE_SCOPE("Resize SwapChains");

        TArray<FSwapChainResizeInfo> ResizeRequests;
        {
            TScopedLock Lock(SwapChainsToResizeCS);
            ResizeRequests = ::Move(SwapChainsToResize);
            SwapChainsToResize.Clear();
        }

        for (const FSwapChainResizeInfo& ResizeInfo : ResizeRequests)
        {
            if (ResizeInfo.HasPendingChange())
            {
                CommandList.ResizeSwapChain(ResizeInfo.SwapChain.Get(), ResizeInfo.Width, ResizeInfo.Height, ResizeInfo.Format, ResizeInfo.ColorSpace);
            }
        }
    }

    // Begin capture GPU FrameTime
    FGPUProfiler::Get().BeginGPUFrame(CommandList);

    if (Packet.SwapChain)
    {
        TRACE_SCOPE("Prepare SwapChain");

        FRHITexture* BackBuffer = Packet.SwapChain->GetBackBuffer();
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::Present, ERHIResourceState::RenderTarget));
    }
}

void FSceneRenderer::RenderThread_RenderSceneFrame(const FSceneRenderPacket& Packet)
{
    CHECK_RENDER_THREAD();

    FScene* CurrentScene = static_cast<FScene*>(Packet.View.Scene);

    RenderThread_BeginSceneCommandList(Packet);

    if (CurrentScene)
    {
        CurrentScene->RenderThread_ApplyAndCull(Packet.View.CameraSnapshot, Packet.View.bHasCamera);
    }

    RenderThread_RenderSceneView(Packet.View, Packet.SelectedObjectIDs);

    FGPUProfiler::Get().EndGPUFrame(CommandList);
    CommandList.PopEvent();

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
}

void FSceneRenderer::RenderThread_PrepareResources(const FSceneRenderView& SceneRenderView, FScene* Scene)
{
    TRACE_SCOPE("PrepareResources");

    if (!SceneRenderView.RenderTarget)
    {
        LOG_WARNING("[FSceneRenderer]: PrepareResources called without an output target; nothing to prepare for this view.");
        return;
    }

    if (RenderSettings::NeedsResize())
    {
        ResizeResources(RenderSettings::GetRenderWidth(), RenderSettings::GetRenderHeight());
    }

    if (RHI::bSupportsRayTracing && RayTracer.NeedsReflectionReconfigure())
    {
        FRHICommandListExecutor::Get().WaitForGPU();
        RayTracer.CreateResources(Resources, Resources.CurrentRenderWidth, Resources.CurrentRenderHeight);
    }

    Resources.BuildLightBuffers(CommandList, Scene);

    RenderThread_PrepareCameraData(SceneRenderView, Scene);

    if (Scene)
    {
        Resources.MaterialData.Clear();

        // TODO: Only do this once?
        for (const FSceneStaticMesh* StaticMesh : Scene->GetStaticMeshes())
        {
            const FVertexDeclaration& Declaration = StaticMesh->Mesh->GetVertexDeclaration();
            for (const TSharedPtr<FMaterial>& MaterialRef : StaticMesh->Materials)
            {
                FMaterial* Material = MaterialRef.Get();
                if (!Material)
                {
                    continue;
                }

            #if EDITOR_BUILD
                EditorNoJitterDepthPass->PreparePipelineState(Material, Declaration, Resources);
                EditorSelectionIDPass->PreparePipelineState(Material, Declaration, Resources);
            #endif

                DepthPrePass->PreparePipelineState(Material, Declaration, Resources);
                BasePass->PreparePipelineState(Material, Declaration, Resources);
            }
        }

        int32 MaterialIndex = 0;
        for (FMaterial* Material : Scene->GetMaterials())
        {
            Material->SetBufferIndex(MaterialIndex++);

            FMaterialHLSL MaterialEntry;
            Material->FillMaterialData(MaterialEntry);
            FillMaterialHandles(*Material, MaterialEntry);
            Resources.MaterialData.Emplace(MaterialEntry);
        }

        if (!Resources.MaterialData.IsEmpty())
        {
            const uint32 RequiredCount = uint32(Resources.MaterialData.Size());
            const uint32 CurrentCount  = Resources.MaterialDataBuffer ? uint32(Resources.MaterialDataBuffer->GetDesc().Size / sizeof(FMaterialHLSL)) : 0;

            if (!Resources.MaterialDataBuffer || RequiredCount > CurrentCount)
            {
                FRHIBufferDesc MaterialBufferDesc;
                MaterialBufferDesc.Stride = sizeof(FMaterialHLSL);
                MaterialBufferDesc.Size   = uint64(MaterialBufferDesc.Stride) * RequiredCount;
                MaterialBufferDesc.Flags  = EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

                Resources.MaterialDataBuffer    = RHI::CreateBuffer(MaterialBufferDesc, ERHIResourceState::GenericRead, nullptr);
                Resources.MaterialDataBufferSRV = nullptr;

                if (Resources.MaterialDataBuffer)
                {
                    Resources.MaterialDataBuffer->SetDebugName("Material Data Buffer");

                    const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateBuffer(0, RequiredCount);
                    Resources.MaterialDataBufferSRV = RHI::CreateShaderResourceView(Resources.MaterialDataBuffer.Get(), SRVDesc);
                }
            }

            if (Resources.MaterialDataBuffer)
            {
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Resources.MaterialDataBuffer.Get(), ERHIResourceState::GenericRead, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(Resources.MaterialDataBuffer.Get(), FBufferRegion(0, sizeof(FMaterialHLSL) * RequiredCount), Resources.MaterialData.Data());
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Resources.MaterialDataBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::GenericRead));
            }
        }
    }

    const EFormat OutputFormat = SceneRenderView.RenderTarget->GetDesc().Format;

#if EDITOR_BUILD
    TonemapPass->PreparePipelineStateForFormat(RendererTextureFormats::SceneTargetFormat);
    FinalCompositePass->PreparePipelineStateForFormat(OutputFormat);
#else
    TonemapPass->PreparePipelineStateForFormat(OutputFormat);
#endif

    FXAAPass->PreparePipelineStateForFormat(OutputFormat);
    DebugRenderer->PreparePipelineStateForFormat(OutputFormat);
    DebugViewPass->PreparePipelineStateForFormat(OutputFormat);
}

void FSceneRenderer::RenderThread_PrepareCameraData(const FSceneRenderView& SceneRenderView, FScene* Scene)
{
    TRACE_SCOPE("PrepareCameraData");

    FSceneCamera* Camera = Scene ? Scene->GetCamera() : nullptr;
    if (!Camera)
    {
        return;
    }

    CameraBuffer.PrevViewProjection          = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjection              = Camera->Snapshot.ViewProjection;
    CameraBuffer.ViewProjectionInv           = Camera->Snapshot.ViewProjectionInverse;
    CameraBuffer.ViewProjectionUnjittered    = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjectionInvUnjittered = CameraBuffer.ViewProjectionInv;
    CameraBuffer.View                        = Camera->Snapshot.View;
    CameraBuffer.ViewInv                     = Camera->Snapshot.ViewInverse;
    CameraBuffer.Projection                  = Camera->Snapshot.Projection;
    CameraBuffer.ProjectionInv               = Camera->Snapshot.ProjectionInverse;
    CameraBuffer.ProjectionUnjittered        = CameraBuffer.Projection;
    CameraBuffer.ProjectionInvUnjittered     = CameraBuffer.ProjectionInv;
    CameraBuffer.PrevPosition                = CameraBuffer.Position;
    CameraBuffer.Position                    = Camera->Snapshot.Position;
    CameraBuffer.Forward                     = Camera->Snapshot.Forward;
    CameraBuffer.Right                       = Camera->Snapshot.Right;
    CameraBuffer.NearPlane                   = Camera->Snapshot.NearPlane;
    CameraBuffer.FarPlane                    = Camera->Snapshot.FarPlane;
    CameraBuffer.AspectRatio                 = Camera->Snapshot.AspectRatio;
    CameraBuffer.ViewportWidth               = float(Resources.CurrentRenderWidth);
    CameraBuffer.ViewportHeight              = float(Resources.CurrentRenderHeight);

    bUseHardwareJitter   = GEnableTemporalAA && GTemporalAAHardwareJitter && RHI::bSupportsProgrammableSamplePositions && IsSampleCountSupported(RHI::SupportedSamplePositionSampleCounts, RHI_SAMPLE_COUNT_1);
    FrameSamplePositions = FRHISamplePositionsDesc();

    CameraBuffer.PrevProjectionJitter = CameraBuffer.ProjectionJitter;

    if (GEnableTemporalAA)
    {
        const Vector2 CameraJitter    = HaltonState.NextSample();
        const Vector2 ClipSpaceJitter = CameraJitter / Vector2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

        if (bUseHardwareJitter)
        {
            // The rasterizer moves the sample instead of the projection, so the matrices
            // stay unjittered and the velocity pass has nothing to subtract back out.
            CameraBuffer.ProjectionJitter = Vector2(0.0f);

            // Moving the sample within the pixel shifts the image the opposite way, so this is the negated
            // jitter. HaltonState.NextSample() returns [-1, 1], so halving lands in [-0.5, 0.5]; the upper 
            // bound is exclusive and both backends quantize to 1/16th of a pixel, so 7/16 is the largest 
            // usable offset.
            constexpr float MinSampleOffset = -0.5f;
            constexpr float MaxSampleOffset = 7.0f / 16.0f;

            FrameSamplePositions.NumSamplesPerPixel = 1;
            FrameSamplePositions.GridWidth          = 1;
            FrameSamplePositions.GridHeight         = 1;
            FrameSamplePositions.Positions[0]       = FRHISamplePosition(
                Math::Clamp(-CameraJitter.X * 0.5f, MinSampleOffset, MaxSampleOffset),
                Math::Clamp(CameraJitter.Y * 0.5f, MinSampleOffset, MaxSampleOffset));
        }
        else
        {
            CameraBuffer.ProjectionJitter = ClipSpaceJitter;

            // Add Jitter to projection matrix
            Matrix4 JitterOffset           = Matrix4::Translation(Vector3(ClipSpaceJitter.X, ClipSpaceJitter.Y, 0.0f));
            CameraBuffer.Projection        = CameraBuffer.Projection * JitterOffset;
            CameraBuffer.ProjectionInv     = CameraBuffer.Projection.GetInverse();

            // Calculate new ViewProjection
            CameraBuffer.ViewProjection    = CameraBuffer.View * CameraBuffer.Projection;
            CameraBuffer.ViewProjectionInv = CameraBuffer.ViewProjection.GetInverse();
        }

        // Both paths land the same offset in the image, so anything reconstructing 
        // from screen coordinates uses this regardless of which one produced it.
        CameraBuffer.ImageJitter = ClipSpaceJitter;
    }
    else
    {
        CameraBuffer.ProjectionJitter = Vector2(0.0f);
        CameraBuffer.ImageJitter      = Vector2(0.0f);
    }

    // Prepare matrices for the GPU
    CameraBuffer.ViewProjection              = CameraBuffer.ViewProjection.GetTranspose();
    CameraBuffer.ViewProjectionInv           = CameraBuffer.ViewProjectionInv.GetTranspose();
    CameraBuffer.ViewProjectionUnjittered    = CameraBuffer.ViewProjectionUnjittered.GetTranspose();
    CameraBuffer.ViewProjectionInvUnjittered = CameraBuffer.ViewProjectionInvUnjittered.GetTranspose();
    CameraBuffer.View                        = CameraBuffer.View.GetTranspose();
    CameraBuffer.ViewInv                     = CameraBuffer.ViewInv.GetTranspose();
    CameraBuffer.Projection                  = CameraBuffer.Projection.GetTranspose();
    CameraBuffer.ProjectionInv               = CameraBuffer.ProjectionInv.GetTranspose();
    CameraBuffer.ProjectionUnjittered        = CameraBuffer.ProjectionUnjittered.GetTranspose();
    CameraBuffer.ProjectionInvUnjittered     = CameraBuffer.ProjectionInvUnjittered.GetTranspose();

    if (SceneRenderView.bCameraCut)
    {
        // Previous-frame matrices are already stored in GPU-transposed form.
        CameraBuffer.PrevViewProjection   = CameraBuffer.ViewProjection;
        CameraBuffer.PrevProjectionJitter = CameraBuffer.ProjectionJitter;
        TemporalAA->InvalidateHistory();
        RayTracer.InvalidateReflectionHistory();
        HaltonState.SampleIndex = 0;
    }

    // Update GPU Camera Buffer
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Resources.CameraBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
    CommandList.UpdateBuffer(Resources.CameraBuffer.Get(), FBufferRegion(0, sizeof(FCameraHLSL)), &CameraBuffer);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Resources.CameraBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));
}

void FSceneRenderer::RenderThread_RenderSceneView(const FSceneRenderView& SceneRenderView, const TArray<uint32>& SelectedObjectIDs)
{
    CHECK_RENDER_THREAD();

#if !EDITOR_BUILD
    UNREFERENCED_VARIABLE(SelectedObjectIDs); // Only consumed by the editor selection-outline pass.
#endif

    FScene* CurrentScene = static_cast<FScene*>(SceneRenderView.Scene);
    RenderThread_PrepareResources(SceneRenderView, CurrentScene);

    if (SceneRenderView.DebugView == FSceneRenderView::EDebugView::RayTracingPrimaryID && RHI::bSupportsRayTracing)
    {
        RayTracer.RenderPrimaryRayDebug(CommandList, Resources, CurrentScene);
        DebugViewPass->Execute(CommandList, SceneRenderView, Resources, FSceneRenderView::EDebugView::RayTracingPrimaryID);
        return;
    }

    const FRHITransitionBarrierDesc GBufferToWrite[] =
    {
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Albedo].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Normal].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Material].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Velocity].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::DepthWrite),
    };

    // D3D12 ties a depth buffer's contents to the sample pattern that produced them.
    const bool bRestorePrevSamplePositions = PrevFrameSamplePositions.NumSamplesPerPixel > 0;
    if (bRestorePrevSamplePositions)
    {
        CommandList.SetSamplePositions(PrevFrameSamplePositions);
    }

    CommandList.TransitionBarrier(GBufferToWrite);

    // Offset the raster sample so the prepass and base pass produce the TAA jitter without a matrix offset.
    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FrameSamplePositions);
    }
    else if (bRestorePrevSamplePositions)
    {
        CommandList.SetSamplePositions(FRHISamplePositionsDesc());
    }

    PrevFrameSamplePositions = bUseHardwareJitter ? FrameSamplePositions : FRHISamplePositionsDesc();

    // PrePass
    if (GPrePassEnabled)
    {
        DepthPrePass->Execute(CommandList, Resources, CurrentScene);
    }
    else
    {
        FRHIDepthStencilView* DepthStencilView = Resources.GBuffer[EGBufferIndex::Depth]->GetDepthStencilView();
        CommandList.ClearDepthStencilView(DepthStencilView, 1.0f, 0);
    }

#if SUPPORT_VARIABLE_RATE_SHADING
    if (ShadingImage && GEnableVariableRateShading && ShadingImage->GetDesc().Extent.X > 0 && ShadingImage->GetDesc().Extent.Y > 0)
    {
        RHI_EVENT_SCOPE(CommandList, "VRS Image");
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(ShadingImage.Get(), ERHIResourceState::ShadingRateSource, ERHIResourceState::UnorderedAccess));

        CommandList.SetComputePipelineState(ShadingRatePipeline.Get());

        FRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        CommandList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

        CommandList.Dispatch(ShadingImage->GetDesc().Extent.X, ShadingImage->GetDesc().Extent.Y, 1);

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(ShadingImage.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::ShadingRateSource));

        CommandList.SetShadingRateImage(ShadingImage.Get());
    }
    else if (RHISupportsVariableRateShading())
    {
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);
    }
#endif

    // BasePass
    if (GBasePassEnabled)
    {
        BasePass->Execute(CommandList, Resources, CurrentScene);
    }

    // Depth Reduce
    if (GCSMTightFrustum)
    {
        DepthReducePass->Execute(CommandList, Resources, CurrentScene);
    }

    const FRHITransitionBarrierDesc GBufferToRead[] =
    {
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Albedo].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Normal].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Velocity].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Material].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::DepthWrite, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.SSAOBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess),
    };

    CommandList.TransitionBarrier(GBufferToRead);

    // The lighting and SSAO work in between is compute, and it transitions other depth resources (shadow maps)
    // that were rendered with the default pattern, so drop back to it until the depth buffer is touched again.
    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FRHISamplePositionsDesc());
    }

    const bool bIsRayTracingActive = IsRayTracingActive();
    if (bIsRayTracingActive)
    {
        if (!bRayTracingWasActive)
        {
            RayTracer.InvalidateReflectionHistory();
        }

        GPU_TRACE_SCOPE(CommandList, "Ray Tracing");
        RayTracer.PreRender(CommandList, Resources, CurrentScene);
    }
    else
    {
        if (bRayTracingWasActive)
        {
            RayTracer.ReleaseRayTracingResources(CurrentScene);
        }
        
        STAT_SET(STAT_RT_Active,                  0);
        STAT_SET(STAT_RT_InstanceCount,           0);
        STAT_SET(STAT_RT_HitGroupCount,           0);
        STAT_SET(STAT_RT_GeometryTableRows,       0);
        STAT_SET(STAT_RT_LazyBLASBuildsThisFrame, 0);
        STAT_SET(STAT_RT_SkippedNullGeometry,     0);
    }

    bRayTracingWasActive = bIsRayTracingActive;

    // SSAO
    if (GEnableSSAO)
    {
        ScreenSpaceOcclusionPass->Execute(CommandList, Resources);
    }
    else
    {
        CommandList.ClearUnorderedAccessViewFloat(Resources.SSAOBuffer->GetUnorderedAccessView(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.SSAOBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));

    // Check for shadow texture resolution changes at runtime
    {
        auto ClampAndSnapPow2 = [](int32 MinSize, int32 MaxSize, int32 Value) -> int32
        {
            return Math::ClosestPowerOfTwo(Math::Clamp(Value, MinSize, MaxSize));
        };

        const int32 NewCascadeSize = ClampAndSnapPow2(512, 4096, GCSMCascadeSize);
        if (NewCascadeSize != Resources.CascadeSize)
        {
            Resources.CascadeSize = NewCascadeSize;
            CascadedShadowsRenderPass->CreateResources(Resources);
        }

        const int32 NewPointLightSize = ClampAndSnapPow2(128, 1024, GPointLightShadowMapSize);
        if (NewPointLightSize != Resources.PointLightShadowSize)
        {
            Resources.PointLightShadowSize = NewPointLightSize;
            PointLightRenderPass->CreateResources(Resources);
        }
    }

    // Render Shadows
    FSceneDirectionalLight* DirectionalLight = CurrentScene ? CurrentScene->GetDirectionalLight() : nullptr;

    const bool bEnableShadows    = GShadowsEnabled;
    const bool bEnableSunShadows = GSunShadowsEnabled && (!DirectionalLight || DirectionalLight->bCastShadows);

    if (bEnableShadows)
    {
        // Point Lights
        if (GPointLightShadowsEnabled)
        {
            PointLightRenderPass->Execute(CommandList, Resources, CurrentScene);
        }

        // Directional Light
        if (bEnableSunShadows)
        {
            if (!GFreezeRendering)
            {
                CascadeGenerationPass->Execute(CommandList, Resources);
            }

            CascadedShadowsRenderPass->Execute(CommandList, Resources, CurrentScene);
        }
    }

    // ShadowMask and GBuffer
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.SceneTarget.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::UnorderedAccess));
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.IntegrationLUT.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource));

    if (CurrentScene)
    {
        if (FSceneSkyLight* SkyLight = CurrentScene->GetSkyLight())
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SkyLight->DiffuseCubeMap.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource));
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SkyLight->SpecularCubeMap.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource));
        }
    }

    // In order to render the shadow-mask, we want all these features to be enabled
    const bool bEnableShadowMask = GShadowMaskEnabled;
    const bool bNeedsCascadeDebug =
        SceneRenderView.DebugView == FSceneRenderView::EDebugView::ShadowCascadeIndex ||
        SceneRenderView.DebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay ||
        SceneRenderView.SecondaryDebugView == FSceneRenderView::EDebugView::ShadowCascadeIndex ||
        SceneRenderView.SecondaryDebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay;

    if (bEnableShadows && bEnableShadowMask && bEnableSunShadows)
    {
        ShadowMaskRenderPass->Execute(CommandList, Resources, bNeedsCascadeDebug);
    }
    else
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.DirectionalShadowMask.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.CascadeIndexBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));

        const Vector4 MaskClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        CommandList.ClearUnorderedAccessViewFloat(Resources.DirectionalShadowMask->GetUnorderedAccessView(), MaskClearColor);

        const Vector4 DebugClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.ClearUnorderedAccessViewFloat(Resources.CascadeIndexBuffer->GetUnorderedAccessView(), DebugClearColor);

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.CascadeIndexBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.DirectionalShadowMask.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
    }

    // Main LightPass
    TiledLightPass->Execute(CommandList, Resources, CurrentScene);

    // Moved ahead of the depth transitions below so no other depth resource is transitioned while the scene
    // depth's sample pattern is bound.
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.PointLightShadowMaps.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));

    // The skybox depth-tests against the jittered depth buffer, so it belongs inside the jittered window
    // together with the transitions that carry that buffer through to the TAA resolve.
    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FrameSamplePositions);
    }

    // The skybox pass binds depth as a ReadOnlyDepth DSV (bDepthWriteEnable = false)
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::DepthRead));
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.SceneTarget.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::RenderTarget));

    // Skybox Pass
    if (GSkyboxEnabled)
    {
        SkyboxRenderPass->Execute(CommandList, Resources, CurrentScene);
    }


    if (CurrentScene)
    {
        if (FSceneSkyLight* SkyLight = CurrentScene->GetSkyLight())
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SkyLight->DiffuseCubeMap.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SkyLight->SpecularCubeMap.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));
        }
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.IntegrationLUT.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));

    // Forward Pass
    // if (!Resources.ForwardVisibleCommands.IsEmpty())
    // {
        // ForwardPass->Execute(CommandList, Resources, Scene);
    // }

    // Temporal AA
    if (GEnableTemporalAA)
    {
        // Source state matches the ReadOnlyDepth transition done before the skybox pass above.
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::DepthRead, ERHIResourceState::NonPixelShaderResource));
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.SceneTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::UnorderedAccess));

        TemporalAA->Execute(CommandList, Resources);

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.SceneTarget.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::PixelShaderResource));
    }
    else
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::DepthRead, ERHIResourceState::PixelShaderResource));
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.SceneTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::PixelShaderResource));
    }

    // The scene depth is done being written and transitioned, so drop back to the default pattern before the
    // editor and UI passes. FEditorNoJitterDepthPass in particular exists to give picking a stable depth buffer.
    if (bUseHardwareJitter)
    {
        CommandList.SetSamplePositions(FRHISamplePositionsDesc());
    }

#if EDITOR_BUILD
    {
        EditorNoJitterDepthPass->Execute(CommandList, Resources, CurrentScene); 
        EditorSelectionIDPass->Execute(CommandList, Resources, CurrentScene); 
 
        RenderThread_ProcessEditorObjectPickRequests(CommandList, Resources, CurrentScene); 
 
        SelectionOutlinePass->Execute(CommandList, Resources, SelectedObjectIDs); 
    } 
#endif 

    // FXAA
    if (GEnableFXAA)
    {
        FXAAPass->Execute(CommandList, SceneRenderView, Resources);
    }

    // Perform ToneMapping and output to BackBuffer
#if EDITOR_BUILD
    TonemapPass->Execute(CommandList, Resources, Resources.TonemappedTarget.Get(), false);
    FinalCompositePass->Execute(CommandList, SceneRenderView, Resources);
#else
    TonemapPass->Execute(CommandList, Resources, SceneRenderView.RenderTarget, true);
#endif

    // Debug geometry draws after composite so they render on top of the editor grid
    {
        const bool bAnyDebugDraw = GDrawPointLights || GDrawLightProbes || GDrawAABBs;
        if (bAnyDebugDraw)
        {
        #if EDITOR_BUILD
            FRHITexture* DebugDepthTarget = Resources.EditorNoJitterDepth.Get();
            const bool   bDebugDepthIsJittered = false;
        #else
            FRHITexture* DebugDepthTarget = Resources.GBuffer[EGBufferIndex::Depth].Get();
            // Outside the editor the debug geometry shares the jittered scene depth, so its transitions and
            // draws have to keep agreeing with the pattern that buffer was rendered with.
            const bool   bDebugDepthIsJittered = PrevFrameSamplePositions.NumSamplesPerPixel > 0;
        #endif

            if (bDebugDepthIsJittered)
            {
                CommandList.SetSamplePositions(PrevFrameSamplePositions);
            }

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SceneRenderView.RenderTarget, ERHIResourceState::RenderTarget));

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DebugDepthTarget, ERHIResourceState::PixelShaderResource, ERHIResourceState::DepthWrite));

            if (GDrawPointLights)
            {
                DebugRenderer->RenderPointLights(CommandList, Resources, CurrentScene, SceneRenderView.RenderTarget, DebugDepthTarget);
            }
            
            if (GDrawLightProbes)
            {
                DebugRenderer->RenderLightProbes(CommandList, Resources, CurrentScene, SceneRenderView.RenderTarget, DebugDepthTarget);
            }

            if (GDrawAABBs)
            {
                DebugRenderer->RenderObjectAABBs(CommandList, Resources, CurrentScene, SceneRenderView.RenderTarget, DebugDepthTarget);
            }

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DebugDepthTarget, ERHIResourceState::DepthWrite, ERHIResourceState::PixelShaderResource));

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SceneRenderView.RenderTarget, ERHIResourceState::PixelShaderResource));

            if (bDebugDepthIsJittered)
            {
                CommandList.SetSamplePositions(FRHISamplePositionsDesc());
            }
        }
    }

    if (SceneRenderView.DebugView != FSceneRenderView::EDebugView::None)
    {
        DebugViewPass->Execute(CommandList, SceneRenderView, Resources, SceneRenderView.DebugView);
    }

    if (SceneRenderView.SecondaryDebugView != FSceneRenderView::EDebugView::None)
    {
        FRHITexture* RenderTarget = SceneRenderView.RenderTarget;
        if (RenderTarget)
        {
            const int32 TargetWidth   = static_cast<int32>(RenderTarget->GetDesc().Extent.X);
            const int32 TargetHeight  = static_cast<int32>(RenderTarget->GetDesc().Extent.Y);
            const int32 OverlayWidth  = Math::Max(TargetWidth / 2, 1);
            const int32 OverlayHeight = Math::Max(TargetHeight / 2, 1);
            const int32 OverlayX      = TargetWidth - OverlayWidth;
            const int32 OverlayY      = 0;

            DebugViewPass->ExecuteOverlay(CommandList, SceneRenderView, Resources, SceneRenderView.SecondaryDebugView, OverlayX, OverlayY, OverlayWidth, OverlayHeight);
        }
    }

} 
 
#if EDITOR_BUILD
void FSceneRenderer::RenderThread_ProcessEditorObjectPickRequests(FRHICommandList& InCommandList, FFrameResources& InResources, FScene* CurrentScene)
{
    // Optional editor picking: copy a small region from the ObjectID render target to a readback buffer and signal a fence.
    {
        TScopedLock Lock(ObjectPickStateCS);
        if (InFlightObjectPicks.Size() >= static_cast<int32>(MaxInFlightObjectPicks))
        {
            return;
        }
    }

    FEditorObjectPickRequest Request;
    if (!PendingObjectPicks.Dequeue(Request))
    {
        return;
    }

    if (Request.Scene != CurrentScene || !InResources.EditorObjectID_NoJitter)
    {
        return;
    }

    const uint32 TexWidth  = InResources.EditorObjectID_NoJitter->GetDesc().Extent.X;
    const uint32 TexHeight = InResources.EditorObjectID_NoJitter->GetDesc().Extent.Y;
    if (TexWidth == 0 || TexHeight == 0)
    {
        return;
    }

    const uint32 PixelX = Math::Min(Request.PixelX, TexWidth - 1);
    const uint32 PixelY = Math::Min(Request.PixelY, TexHeight - 1);

    const uint32 BytesPerPixel = GetByteStrideFromFormat(InResources.EditorObjectID_NoJitter->GetDesc().Format);
    if (BytesPerPixel == 0)
    {
        // Unsupported format; skip pick.
        return;
    }

    const int32 RequestedRadius = GEditorPickSearchRadius;
    const int32 SampleRadius    = Math::Clamp(RequestedRadius, 0, 64);
    const bool  bIsRect         = Request.bIsRect;

    // The flipped-Y window is a click-tolerance debug aid for point picks and means nothing for a box
    const bool  bTryFlipY = GEditorPickTryFlipY && !bIsRect;

    int32 X0 = 0;
    int32 X1 = 0;
    int32 Y0 = 0;
    int32 Y1 = 0;

    if (bIsRect)
    {
        X0 = int32(PixelX);
        Y0 = int32(PixelY);
        X1 = int32(Math::Clamp(Math::Max(Request.MaxX, Request.PixelX), PixelX, TexWidth - 1));
        Y1 = int32(Math::Clamp(Math::Max(Request.MaxY, Request.PixelY), PixelY, TexHeight - 1));
    }
    else
    {
        X0 = Math::Clamp<int32>(int32(PixelX) - SampleRadius, 0, int32(TexWidth) - 1);
        X1 = Math::Clamp<int32>(int32(PixelX) + SampleRadius, 0, int32(TexWidth) - 1);
        Y0 = Math::Clamp<int32>(int32(PixelY) - SampleRadius, 0, int32(TexHeight) - 1);
        Y1 = Math::Clamp<int32>(int32(PixelY) + SampleRadius, 0, int32(TexHeight) - 1);
    }

    const uint32 SourceHeight = uint32((Y1 - Y0) + 1);

    // Reading back every row of a full-viewport box would cost tens of megabytes, so tall boxes step over source rows
    const uint32 MaxRows = bIsRect ? uint32(Math::Max(GEditorPickMaxRectRows, 1)) : SourceHeight;
    const uint32 RowStep = Math::Max(1u, (SourceHeight + MaxRows - 1) / MaxRows);

    const uint32 RegionWidth  = uint32((X1 - X0) + 1);
    const uint32 RegionHeight = (SourceHeight + RowStep - 1) / RowStep;
    const uint32 CenterLocalX = uint32(int32(PixelX) - X0);
    const uint32 CenterLocalY = uint32(int32(PixelY) - Y0);

    const uint64 NormalRowPitchBytes  = Math::AlignUp<uint64>(uint64(BytesPerPixel) * uint64(RegionWidth), 256ull);
    const uint64 NormalRowStrideBytes = Math::AlignUp<uint64>(NormalRowPitchBytes, 512ull);
    const uint64 NormalRequiredSize   = NormalRowStrideBytes * uint64(RegionHeight);

    uint64 FlippedRowStrideBytes = 0;
    uint64 FlippedRequiredSize   = 0;
    uint64 FlippedBaseOffset     = 0;
    uint32 FlippedCenterLocalX   = 0;
    uint32 FlippedCenterLocalY   = 0;
    uint32 FlippedRegionWidth    = 0;
    uint32 FlippedRegionHeight   = 0;
    int32  FlipX0                = 0;
    int32  FlipY0                = 0;

    if (bTryFlipY)
    {
        const uint32 FlippedPixelY = (TexHeight > 0) ? (TexHeight - 1 - PixelY) : PixelY;

        const int32 FY0 = Math::Clamp<int32>(int32(FlippedPixelY) - SampleRadius, 0, int32(TexHeight) - 1);
        const int32 FY1 = Math::Clamp<int32>(int32(FlippedPixelY) + SampleRadius, 0, int32(TexHeight) - 1);

        const int32 FX0 = X0;
        const int32 FX1 = X1;

        FlippedRegionWidth  = uint32((FX1 - FX0) + 1);
        FlippedRegionHeight = uint32((FY1 - FY0) + 1);
        FlippedCenterLocalX = uint32(int32(PixelX) - FX0);
        FlippedCenterLocalY = uint32(int32(FlippedPixelY) - FY0);

        const uint64 FlippedRowPitchBytes = Math::AlignUp<uint64>(uint64(BytesPerPixel) * uint64(FlippedRegionWidth), 256ull);
        FlippedRowStrideBytes = Math::AlignUp<uint64>(FlippedRowPitchBytes, 512ull);
        FlippedRequiredSize   = FlippedRowStrideBytes * uint64(FlippedRegionHeight);
        FlippedBaseOffset     = Math::AlignUp<uint64>(NormalRequiredSize, 512ull);

        FlipX0 = FX0;
        FlipY0 = FY0;
    }

    // D32_Float and R32_Uint are both 4 bytes over the same rectangle, so the depth window reuses the normal row stride.
    const bool   bCopyDepth        = InResources.EditorNoJitterDepth.IsValid();
    const uint64 WindowsEnd        = bTryFlipY ? (FlippedBaseOffset + FlippedRequiredSize) : NormalRequiredSize;
    const uint64 DepthBaseOffset   = Math::AlignUp<uint64>(WindowsEnd, 512ull);
    const uint64 DepthRequiredSize = bCopyDepth ? (NormalRowStrideBytes * uint64(RegionHeight)) : 0ull;

    FRHIBufferDesc ReadbackDesc;
    ReadbackDesc.Flags  = EBufferFlags::ReadBack;
    ReadbackDesc.Stride = BytesPerPixel;
    ReadbackDesc.Size   = bCopyDepth ? (DepthBaseOffset + DepthRequiredSize) : WindowsEnd;

    FRHIFenceRef  Fence          = RHI::CreateFence();
    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(ReadbackDesc, ERHIResourceState::CopyDest, nullptr);

    if (!(ReadbackBuffer && Fence))
    {
        return;
    }

    Fence->SetDebugName("EditorObjectPick Fence");
    ReadbackBuffer->SetDebugName("EditorObjectPick Readback");

    if (GEditorPickDebug)
    {
        LOG_INFO("[EditorPick] Request. Rect=%s Pixel=(%u,%u) Tex=%ux%u Radius=%d TryFlipY=%s Region=(%u,%u) RowStep=%u CenterLocal=(%u,%u)",
            bIsRect ? "true" : "false", PixelX, PixelY, TexWidth, TexHeight, SampleRadius, bTryFlipY ? "true" : "false",
            RegionWidth, RegionHeight, RowStep, CenterLocalX, CenterLocalY);
    }

    InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(InResources.EditorObjectID_NoJitter.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::CopySource));

    // Copy a rectangular neighborhood into a readback buffer.
    // We do one copy per row to control destination row stride across backends and keep D3D12 offsets 512-byte aligned.
    for (uint32 Row = 0; Row < RegionHeight; ++Row)
    {
        const uint64 DstOffset  = NormalRowStrideBytes * uint64(Row);
        const uint32 SourceRowY = uint32(Y0) + (Row * RowStep);
        InCommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), DstOffset, InResources.EditorObjectID_NoJitter.Get(), FTextureRegion2D(RegionWidth, 1, uint32(X0), SourceRowY), 0);
    }

    if (bTryFlipY)
    {
        for (uint32 Row = 0; Row < FlippedRegionHeight; ++Row)
        {
            const uint64 DstOffset = FlippedBaseOffset + (FlippedRowStrideBytes * uint64(Row));
            InCommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), DstOffset, InResources.EditorObjectID_NoJitter.Get(), FTextureRegion2D(FlippedRegionWidth, 1, uint32(FlipX0), uint32(FlipY0) + Row), 0);
        }
    }

    InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(InResources.EditorObjectID_NoJitter.Get(), ERHIResourceState::CopySource, ERHIResourceState::PixelShaderResource));

    // Same rectangle out of the stable depth buffer, so the pick also reports where the surface is.
    if (bCopyDepth)
    {
        InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(InResources.EditorNoJitterDepth.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::CopySource));

        for (uint32 Row = 0; Row < RegionHeight; ++Row)
        {
            const uint64 DstOffset = DepthBaseOffset + (NormalRowStrideBytes * uint64(Row));
            InCommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), DstOffset, InResources.EditorNoJitterDepth.Get(), FTextureRegion2D(RegionWidth, 1, uint32(X0), uint32(Y0) + Row), 0);
        }

        InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(InResources.EditorNoJitterDepth.Get(), ERHIResourceState::CopySource, ERHIResourceState::PixelShaderResource));
    }

    InCommandList.WriteFence(Fence.Get());

    FEditorObjectPickInFlight InFlight;
    InFlight.Scene                 = CurrentScene;
    InFlight.RequestId             = Request.RequestId;
    InFlight.ReadbackBuffer        = ReadbackBuffer;
    InFlight.Fence                 = Fence;
    InFlight.SampleRadius          = uint32(SampleRadius);
    InFlight.bIsRectPick           = bIsRect ? 1u : 0u;
    InFlight.PixelX                = PixelX;
    InFlight.PixelY                = PixelY;
    InFlight.TexWidth              = TexWidth;
    InFlight.TexHeight             = TexHeight;
    InFlight.NormalBaseOffset      = 0;
    InFlight.NormalRowStrideBytes  = uint32(NormalRowStrideBytes);
    InFlight.NormalWidth           = RegionWidth;
    InFlight.NormalHeight          = RegionHeight;
    InFlight.NormalCenterX         = CenterLocalX;
    InFlight.NormalCenterY         = CenterLocalY;
    InFlight.bHasFlippedWindow     = bTryFlipY ? 1u : 0u;
    InFlight.FlippedBaseOffset     = uint32(FlippedBaseOffset);
    InFlight.FlippedRowStrideBytes = uint32(FlippedRowStrideBytes);
    InFlight.FlippedWidth          = FlippedRegionWidth;
    InFlight.FlippedHeight         = FlippedRegionHeight;
    InFlight.FlippedCenterX        = FlippedCenterLocalX;
    InFlight.FlippedCenterY        = FlippedCenterLocalY;
    InFlight.bHasDepthWindow       = bCopyDepth ? 1u : 0u;
    InFlight.DepthBaseOffset       = uint32(DepthBaseOffset);
    InFlight.DepthRowStrideBytes   = uint32(NormalRowStrideBytes);

    {
        TScopedLock Lock(ObjectPickStateCS);
        InFlightObjectPicks.Add(Move(InFlight));
    }
}
#endif

void FSceneRenderer::RequestEditorObjectPick(FScene* Scene, uint32 PixelX, uint32 PixelY, uint64 RequestId)
{  
#if EDITOR_BUILD 
    if (Scene)  
    { 
        PendingObjectPicks.Enqueue(FEditorObjectPickRequest{ Scene, PixelX, PixelY, PixelX, PixelY, false, RequestId });
    } 
#else
    UNREFERENCED_VARIABLE(Scene);
    UNREFERENCED_VARIABLE(PixelX);
    UNREFERENCED_VARIABLE(PixelY);
    UNREFERENCED_VARIABLE(RequestId);
#endif
} 

void FSceneRenderer::RequestEditorObjectPickRect(FScene* Scene, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
{
#if EDITOR_BUILD
    if (Scene)
    {
        // The corners arrive in drag order, so a box dragged up or to the left has to be normalized first
        const uint32 Left   = Math::Min(MinX, MaxX);
        const uint32 Right  = Math::Max(MinX, MaxX);
        const uint32 Top    = Math::Min(MinY, MaxY);
        const uint32 Bottom = Math::Max(MinY, MaxY);

        // A rect pick has no purpose to route back, so it carries no request id
        PendingObjectPicks.Enqueue(FEditorObjectPickRequest{ Scene, Left, Top, Right, Bottom, true, 0 });
    }
#else
    UNREFERENCED_VARIABLE(Scene);
    UNREFERENCED_VARIABLE(MinX);
    UNREFERENCED_VARIABLE(MinY);
    UNREFERENCED_VARIABLE(MaxX);
    UNREFERENCED_VARIABLE(MaxY);
#endif
}
 
bool FSceneRenderer::PollEditorObjectPickResult(FScene* Scene, FEditorPickResult& OutResult)
{ 
#if EDITOR_BUILD
    if (!Scene) 
    { 
        return false; 
    } 

    // InFlightObjectPicks is mutated on the render thread (ProcessEditorObjectPickRequests).
    TScopedLock Lock(ObjectPickStateCS);

    for (int32 Index = 0; Index < InFlightObjectPicks.Size(); ++Index)
    {
        const FEditorObjectPickInFlight& InFlight = InFlightObjectPicks[Index];
        if (InFlight.Scene != Scene || InFlight.bIsRectPick)
        {
            continue;
        }

        if (InFlight.Fence && InFlight.Fence->IsSignaled() && InFlight.ReadbackBuffer)
        {
            const uint32 BytesPerPixel = InFlight.ReadbackBuffer->GetDesc().Stride ? InFlight.ReadbackBuffer->GetDesc().Stride : sizeof(uint32);
            const uint64 BufferSize    = InFlight.ReadbackBuffer->GetDesc().Size;

            uint32 ObjectID = 0;
            if (BytesPerPixel >= sizeof(uint32) && BufferSize >= sizeof(uint32))
            {
                if (void* Data = InFlight.ReadbackBuffer->Map(0, BufferSize))
                {
                    const uint8* Base = reinterpret_cast<const uint8*>(Data);

                    auto ReadPixel = [&](uint32 WindowBaseOffsetBytes, uint32 RowPitch, uint32 Width, uint32 Height, uint32 X, uint32 Y) -> uint32
                    {
                        if (Width == 0 || Height == 0 || X >= Width || Y >= Height)
                        {
                            return 0;
                        }

                        const uint64 Offset = uint64(WindowBaseOffsetBytes) + (uint64(RowPitch) * uint64(Y)) + (uint64(BytesPerPixel) * uint64(X));
                        if (Offset + sizeof(uint32) > BufferSize || Offset < WindowBaseOffsetBytes)
                        {
                            return 0;
                        }

                        return *reinterpret_cast<const uint32*>(Base + Offset);
                    };

                    auto ChooseFromWindow = [&](uint32 WindowBaseOffsetBytes, uint32 RowPitch, uint32 Width, uint32 Height, uint32 CenterX, uint32 CenterY) -> uint32
                    {
                        uint32 ResultID = ReadPixel(WindowBaseOffsetBytes, RowPitch, Width, Height, CenterX, CenterY);

                        if (ResultID == 0 && Width > 0 && Height > 0)
                        {
                            uint32 BestObjectID = 0;
                            int32  BestDist2    = INT32_MAX;

                            for (uint32 Y = 0; Y < Height; ++Y)
                            {
                                for (uint32 X = 0; X < Width; ++X)
                                {
                                    if (X == CenterX && Y == CenterY)
                                    {
                                        continue;
                                    }

                                    const uint32 SampleObjectID = ReadPixel(WindowBaseOffsetBytes, RowPitch, Width, Height, X, Y);
                                    if (SampleObjectID == 0)
                                    {
                                        continue;
                                    }

                                    const int32 Dx = int32(X) - int32(CenterX);
                                    const int32 Dy = int32(Y) - int32(CenterY);
                                    const int32 Dist2 = Dx * Dx + Dy * Dy;

                                    if (Dist2 < BestDist2)
                                    {
                                        BestDist2    = Dist2;
                                        BestObjectID = SampleObjectID;
                                    }
                                }
                            }

                            ResultID = BestObjectID;
                        }

                        return ResultID;
                    };

                    ObjectID = ChooseFromWindow(
                        InFlight.NormalBaseOffset,
                        InFlight.NormalRowStrideBytes,
                        InFlight.NormalWidth,
                        InFlight.NormalHeight,
                        InFlight.NormalCenterX,
                        InFlight.NormalCenterY);

                    const bool bUsedFlipped = (ObjectID == 0) && (InFlight.bHasFlippedWindow != 0);
                    if (bUsedFlipped)
                    {
                        ObjectID = ChooseFromWindow(
                            InFlight.FlippedBaseOffset,
                            InFlight.FlippedRowStrideBytes,
                            InFlight.FlippedWidth,
                            InFlight.FlippedHeight,
                            InFlight.FlippedCenterX,
                            InFlight.FlippedCenterY);
                    }
                    
                    auto ChooseDepthFromWindow = [&](uint32 CenterX, uint32 CenterY, float& OutDepth) -> bool
                    {
                        auto ReadDepth = [&](uint32 X, uint32 Y) -> float
                        {
                            const uint32 Bits = ReadPixel(InFlight.DepthBaseOffset, InFlight.DepthRowStrideBytes, InFlight.NormalWidth, InFlight.NormalHeight, X, Y);
                            return BitCast<float>(Bits);
                        };

                        const float CenterDepth = ReadDepth(CenterX, CenterY);
                        if (CenterDepth > 0.0f && CenterDepth < 1.0f)
                        {
                            OutDepth = CenterDepth;
                            return true;
                        }

                        // Nearest valid sample, mirroring the ObjectID fallback.
                        float BestDepth = 1.0f;
                        int32 BestDist2 = INT32_MAX;

                        for (uint32 Y = 0; Y < InFlight.NormalHeight; ++Y)
                        {
                            for (uint32 X = 0; X < InFlight.NormalWidth; ++X)
                            {
                                const float SampleDepth = ReadDepth(X, Y);
                                if (SampleDepth <= 0.0f || SampleDepth >= 1.0f)
                                {
                                    continue;
                                }

                                const int32 Dx    = int32(X) - int32(CenterX);
                                const int32 Dy    = int32(Y) - int32(CenterY);
                                const int32 Dist2 = Dx * Dx + Dy * Dy;

                                if (Dist2 < BestDist2)
                                {
                                    BestDist2 = Dist2;
                                    BestDepth = SampleDepth;
                                }
                            }
                        }

                        OutDepth = BestDepth;
                        return BestDist2 != INT32_MAX;
                    };

                    if (InFlight.bHasDepthWindow != 0)
                    {
                        OutResult.bHasDepth = ChooseDepthFromWindow(InFlight.NormalCenterX, InFlight.NormalCenterY, OutResult.DeviceDepth);
                    }

                    if (GEditorPickDebug)
                    {
                        const uint32 NormalCenter = ReadPixel(
                            InFlight.NormalBaseOffset,
                            InFlight.NormalRowStrideBytes,
                            InFlight.NormalWidth,
                            InFlight.NormalHeight,
                            InFlight.NormalCenterX,
                            InFlight.NormalCenterY);

                        const uint32 FlippedCenter = (InFlight.bHasFlippedWindow != 0) ? ReadPixel(
                            InFlight.FlippedBaseOffset,
                            InFlight.FlippedRowStrideBytes,
                            InFlight.FlippedWidth,
                            InFlight.FlippedHeight,
                            InFlight.FlippedCenterX,
                            InFlight.FlippedCenterY) : 0u;

                        auto ComputeStats = [&](uint32 BaseOffset, uint32 RowPitch, uint32 Width, uint32 Height, uint32& OutNonZero)
                        {
                            OutNonZero = 0;

                            for (uint32 Y = 0; Y < Height; ++Y)
                            {
                                for (uint32 X = 0; X < Width; ++X)
                                {
                                    const uint32 Value = ReadPixel(BaseOffset, RowPitch, Width, Height, X, Y);
                                    OutNonZero += (Value != 0) ? 1u : 0u;
                                }
                            }
                        };

                        uint32 NormalNonZero = 0;
                        ComputeStats(InFlight.NormalBaseOffset, InFlight.NormalRowStrideBytes, InFlight.NormalWidth, InFlight.NormalHeight, NormalNonZero);

                        uint32 FlippedNonZero = 0;
                        if (InFlight.bHasFlippedWindow != 0)
                        {
                            ComputeStats(InFlight.FlippedBaseOffset, InFlight.FlippedRowStrideBytes, InFlight.FlippedWidth, InFlight.FlippedHeight, FlippedNonZero);
                        }

                        LOG_INFO("[EditorPick] Readback. Pixel=(%u,%u) Tex=%ux%u Radius=%u NormalCenter=%u FlippedCenter=%u Result=%u UsedFlip=%s",
                            InFlight.PixelX, InFlight.PixelY, InFlight.TexWidth, InFlight.TexHeight, InFlight.SampleRadius, NormalCenter, FlippedCenter, ObjectID, bUsedFlipped ? "true" : "false");
                        LOG_INFO("[EditorPick] ReadbackStats. NormalNonZero=%u FlippedNonZero=%u",
                            NormalNonZero, FlippedNonZero);
                    }

                    InFlight.ReadbackBuffer->Unmap(0, BufferSize);
                }
            }

            OutResult.RequestId = InFlight.RequestId;
            OutResult.ObjectID  = ObjectID;

            InFlightObjectPicks.RemoveAtSwap(Index);
            return true;
        }
    }
 
    return false; 
#else
    UNREFERENCED_VARIABLE(Scene);
    OutResult = FEditorPickResult();
    return false;
#endif
} 

bool FSceneRenderer::PollEditorObjectPickRectResult(FScene* Scene, TArray<uint32>& OutObjectIDs)
{
#if EDITOR_BUILD
    if (!Scene)
    {
        return false;
    }

    // InFlightObjectPicks is mutated on the render thread (ProcessEditorObjectPickRequests).
    TScopedLock Lock(ObjectPickStateCS);

    for (int32 Index = 0; Index < InFlightObjectPicks.Size(); ++Index)
    {
        const FEditorObjectPickInFlight& InFlight = InFlightObjectPicks[Index];
        if (InFlight.Scene != Scene || !InFlight.bIsRectPick)
        {
            continue;
        }

        if (!InFlight.Fence || !InFlight.Fence->IsSignaled() || !InFlight.ReadbackBuffer)
        {
            continue;
        }

        OutObjectIDs.Clear();

        const uint32 BytesPerPixel = InFlight.ReadbackBuffer->GetDesc().Stride ? InFlight.ReadbackBuffer->GetDesc().Stride : sizeof(uint32);
        const uint64 BufferSize    = InFlight.ReadbackBuffer->GetDesc().Size;

        if (BytesPerPixel >= sizeof(uint32) && BufferSize >= sizeof(uint32))
        {
            if (void* Data = InFlight.ReadbackBuffer->Map(0, BufferSize))
            {
                const uint8* Base = reinterpret_cast<const uint8*>(Data);

                // A full-viewport box is millions of pixels, so the dedup is hashed rather than a linear scan
                TSet<uint32> SeenObjectIDs;

                // Neighbouring pixels almost always belong to the same object, so a run skips the lookup entirely
                uint32 LastObjectID = 0;

                // Every distinct object with a visible pixel inside the box is selected, which is what makes a box
                // select respect occlusion for free: an actor hidden behind geometry never wrote a pixel here
                for (uint32 Y = 0; Y < InFlight.NormalHeight; ++Y)
                {
                    for (uint32 X = 0; X < InFlight.NormalWidth; ++X)
                    {
                        const uint64 Offset = uint64(InFlight.NormalBaseOffset) + (uint64(InFlight.NormalRowStrideBytes) * uint64(Y)) + (uint64(BytesPerPixel) * uint64(X));
                        if (Offset + sizeof(uint32) > BufferSize)
                        {
                            continue;
                        }

                        const uint32 ObjectID = *reinterpret_cast<const uint32*>(Base + Offset);
                        if (ObjectID == 0 || ObjectID == LastObjectID)
                        {
                            continue;
                        }

                        LastObjectID = ObjectID;

                        bool bAlreadySeen = false;
                        SeenObjectIDs.Add(ObjectID, &bAlreadySeen);

                        if (!bAlreadySeen)
                        {
                            OutObjectIDs.Add(ObjectID);
                        }
                    }
                }

                InFlight.ReadbackBuffer->Unmap(0, BufferSize);
            }
        }

        if (GEditorPickDebug)
        {
            LOG_INFO("[EditorPick] RectReadback. Region=(%u,%u) UniqueIDs=%d",
                InFlight.NormalWidth, InFlight.NormalHeight, OutObjectIDs.Size());
        }

        InFlightObjectPicks.RemoveAtSwap(Index);
        return true;
    }

    return false;
#else
    UNREFERENCED_VARIABLE(Scene);
    OutObjectIDs.Clear();
    return false;
#endif
}

void FSceneRenderer::RecordUI()
{
    CHECK_MAIN_THREAD();

    RHI_EVENT_SCOPE(UICommandList, "UI Render");

    {
        TRACE_SCOPE("Record UI");

    #if SUPPORT_VARIABLE_RATE_SHADING
        if (RHISupportsVariableRateShading())
        {
            UICommandList.SetShadingRate(EShadingRate::VRS_1x1);
            UICommandList.SetShadingRateImage(nullptr);
        }
    #endif

        if (IImguiPlugin::IsEnabled())
        {
            IImguiPlugin::Get().Draw(UICommandList);
        }
    }
}

void FSceneRenderer::SubmitUIAndPresent(const FSceneRenderPacket& Packet)
{
    CHECK_MAIN_THREAD();

    {
        RHI_EVENT_SCOPE(UICommandList, "UI Viewports");
        TRACE_SCOPE("Render UI Viewports");

        if (IImguiPlugin::IsEnabled())
        {
            IImguiPlugin::Get().DrawViewports(UICommandList);
        }
    }

    if (Packet.SwapChain)
    {
        TRACE_SCOPE("Present SwapChain");

        FRHITexture* BackBuffer = Packet.SwapChain->GetBackBuffer();
        UICommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::RenderTarget, ERHIResourceState::Present));
        UICommandList.PresentSwapChain(Packet.SwapChain.Get(), GVSyncEnabled);
    }

    UICommandList.EndFrame();

    UICommandList.FlushDeletedResources();

    {
        TRACE_SCOPE("ExecuteCommandList");

        if (LastFrameFinishedEvent)
        {
            LastFrameFinishedEvent->Wait(FTimespan::Infinity());
            FPlatformEvent::Recycle(LastFrameFinishedEvent);
            LastFrameFinishedEvent = nullptr;
        }

        LastFrameFinishedEvent = FPlatformEvent::Create(false);
        if (LastFrameFinishedEvent)
        {
            UICommandList.SetEvent(LastFrameFinishedEvent);
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(UICommandList);
    }
}

void FSceneRenderer::ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 InWidth, uint32 InHeight, EFormat InFormat, EColorSpace InColorSpace)
{
    if (!SwapChain)
    {
        return;
    }

    // Queued from the main thread (window/ImGui resize callbacks); consumed on the render thread.
    TScopedLock Lock(SwapChainsToResizeCS);

    for (FSwapChainResizeInfo& Existing : SwapChainsToResize)
    {
        if (Existing.SwapChain == SwapChain)
        {
            if (InWidth  > 0u)
            {
                Existing.Width = InWidth;
            }

            if (InHeight > 0u)
            {
                Existing.Height = InHeight;
            }

            if (InFormat != EFormat::Unknown)
            {
                Existing.Format = InFormat;
            }

            if (InColorSpace != EColorSpace::Unknown)
            {
                Existing.ColorSpace = InColorSpace;
            }

            return;
        }
    }

    SwapChainsToResize.Emplace(SwapChain, InWidth, InHeight, InFormat, InColorSpace);
}

void FSceneRenderer::ResizeResources(uint32 InWidth, uint32 InHeight)
{
    if ((Resources.CurrentRenderWidth != InWidth || Resources.CurrentRenderHeight != InHeight) && InWidth > 0 && InHeight > 0)
    {
        // A recreated depth buffer starts out on the default sample pattern again.
        PrevFrameSamplePositions = FRHISamplePositionsDesc();

        if (!DepthPrePass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!BasePass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!TiledLightPass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!DepthReducePass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!ScreenSpaceOcclusionPass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!ShadowMaskRenderPass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!TemporalAA->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

#if EDITOR_BUILD
        if (!EditorNoJitterDepthPass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (!EditorSelectionIDPass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }
#endif

        if (!TonemapPass->CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

        if (RHI::bSupportsRayTracing && !RayTracer.CreateResources(Resources, InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }

#if EDITOR_BUILD
        if (!SelectionOutlinePass->CreateResources(InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }
#endif

        // Resize ShadingImage if VRS is active (shader/pipeline are unchanged)
        if (ShadingImage && RHI::ShadingRateImageTileSize > 0)
        {
            const uint32 ShadingWidth  = InWidth / RHI::ShadingRateImageTileSize;
            const uint32 ShadingHeight = InHeight / RHI::ShadingRateImageTileSize;

            if (ShadingWidth > 0 && ShadingHeight > 0)
            {
                const ETextureUsageFlags UsageFlags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::ShadingRateTexture;
                FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, ShadingWidth, ShadingHeight, 1, 1, UsageFlags);

                ShadingImage = RHI::CreateTexture(TextureDesc, ERHIResourceState::ShadingRateSource);
                if (ShadingImage)
                {
                    ShadingImage->SetDebugName("Shading Rate Image");
                }
            }
        }

        LOG_INFO("Changed render-resolution. From: w=%d h=%d, To: w=%d h=%d", 
            Resources.CurrentRenderWidth, Resources.CurrentRenderHeight, InWidth, InHeight);
        
        Resources.CurrentRenderWidth  = InWidth;
        Resources.CurrentRenderHeight = InHeight;
        RenderSettings::OnDidChangeRenderResolution(InWidth, InHeight);
    }
}

bool FSceneRenderer::InitShadingImage()
{
    if (RHI::ShadingRateTier != EShadingRateTier::Tier2 || RHI::ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.CurrentRenderWidth / RHI::ShadingRateImageTileSize;
    const uint32 Height = Resources.CurrentRenderHeight / RHI::ShadingRateImageTileSize;

    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::ShadingRateTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, UsageFlags);

    ShadingImage = RHI::CreateTexture(TextureDesc, ERHIResourceState::ShadingRateSource);
    if (!ShadingImage)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadingImage->SetDebugName("Shading Rate Image");
    }

    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadingImage.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    ShadingRateShader = RHI::CreateComputeShader(ShaderCode);
    if (!ShadingRateShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = ShadingRateShader.Get();

    ShadingRatePipeline = RHI::CreateComputePipelineState(PSODesc);
    if (!ShadingRatePipeline)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}
