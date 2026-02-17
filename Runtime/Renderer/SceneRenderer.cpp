#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Time/Timespan.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Engine.h"
#if EDITOR_BUILD
#include "Engine/EditorEngine.h"
#endif
#include "Engine/Resources/Model.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/EditorSelectionRendering.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/RenderSettings.h"

#define SUPPORT_VARIABLE_RATE_SHADING (0)

static TAutoConsoleVariable<bool> CVarEnableSSAO(
    "Renderer.Feature.SSAO",
    "Enables Screen-Space Ambient Occlusion",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEnableFXAA(
    "Renderer.Feature.FXAA",
    "Enables FXAA for Anti-Aliasing",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEnableTemporalAA(
    "Renderer.Feature.TemporalAA",
    "Enables Temporal Anti-Aliasing",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEnableVariableRateShading(
    "Renderer.Feature.VariableRateShading",
    "Enables VRS (Variable Rate Shading)",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPrePassEnabled(
    "Renderer.Feature.PrePass",
    "Enables Pre-Pass",
    true,
    EConsoleVariableFlags::Default);

#if EDITOR_BUILD
static TAutoConsoleVariable<int32> CVarEditorPickSearchRadius(
    "Editor.Pick.SearchRadius",
    "When the center pixel returns ObjectID=0, search within this radius (in pixels) for a non-zero ObjectID.",
    1,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEditorPickTryFlipY( 
    "Editor.Pick.TryFlipY", 
    "Debug-only: If you suspect the pick coordinates are vertically inverted, also sample a vertically flipped Y window when the normal sample returns ObjectID=0.", 
    false, 
    EConsoleVariableFlags::Default); 

static TAutoConsoleVariable<bool> CVarEditorPickDebug(
    "Editor.Pick.Debug",
    "Logs editor picking requests and readback results.",
    false,
    EConsoleVariableFlags::Default);
#endif

static TAutoConsoleVariable<bool> CVarBasePassEnabled(
    "Renderer.Feature.BasePass",
    "Enables BasePass (Disabling this disables most rendering)",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarShadowsEnabled(
    "Renderer.Feature.Shadows",
    "Enables Rendering of ShadowMaps",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarShadowMaskEnabled(
    "Renderer.Feature.ShadowMask",
    "Enables Rendering of ShadowMask for SunShadows",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPointLightShadowsEnabled(
    "Renderer.Feature.PointLightShadows",
    "Enables Rendering of PointLight ShadowMaps",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarSunShadowsEnabled(
    "Renderer.Feature.SunShadows",
    "Enables Rendering of SunLight/DirectionalLight ShadowMaps",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarSkyboxEnabled(
    "Renderer.Feature.Skybox",
    "Enables Rendering of the Skybox",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarDrawAABBs(
    "Renderer.Debug.DrawAABBs",
    "Draws all the objects bounding boxes (AABB)",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarDrawPointLights(
    "Renderer.Debug.DrawPointLights", 
    "Draws all the point-lights as spheres with the light-color",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarDrawLightProbes(
    "Renderer.Debug.LightProbes",
    "Draws all the light-probes as spheres with the cube-map",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarVSyncEnabled(
    "Renderer.Feature.VerticalSync",
    "Enables Vertical-Sync", 
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarFrustumCullEnabled(
    "Renderer.Feature.FrustumCulling",
    "Enables Frustum Culling (CPU) for the main scene and for all shadow frustums",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarRayTracingEnabled(
    "Renderer.Feature.RayTracing",
    "Enables Ray Tracing (Currently broken)",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMTightFrustum(
    "Renderer.CSM.TightFrustum",
    "Set to true to reduce the DepthBuffer to find the Min- and Max Depth in the DepthBuffer to be able to create a tight frustum that fits the scene",
    true,
    EConsoleVariableFlags::Default);

static FAutoConsoleCommand CVarFreezeRendering(
    "Renderer.FreezeRendering",
    "Freezes the updating of Frustum culling",
    FConsoleCommandDelegate::CreateLambda([](FStringView)
    {
        GFreezeRendering = !GFreezeRendering;
    }));

FSceneRenderer::FSceneRenderer()
    : InfoWindow(nullptr)
    , GPUProfilerWindow(nullptr)
    , CommandList()
    , Resources()
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
    , ShadowMaskHistoryPass(nullptr)
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
    , DebugViewPass(nullptr)
#if EDITOR_BUILD
    , FinalCompositePass(nullptr)
#endif
    , LightProbeRenderer(nullptr)
    , DebugRenderer(nullptr)
    , RayTracer(this)
    , ShadingImage(nullptr)
    , ShadingRatePipeline(nullptr)
    , ShadingRateShader(nullptr)
    , TimestampQueries(nullptr)
    , LastFrameFinishedEvent(nullptr)
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
    SAFE_DELETE(ShadowMaskHistoryPass);
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
    SAFE_DELETE(DebugViewPass);
#if EDITOR_BUILD
    SAFE_DELETE(FinalCompositePass);
#endif
    SAFE_DELETE(LightProbeRenderer);
    SAFE_DELETE(DebugRenderer);

    RayTracer.Release();

    Resources.Release();

    ShadingImage.Reset();
    ShadingRatePipeline.Reset();
    ShadingRateShader.Reset();

    TimestampQueries.Reset();

    if (IImguiPlugin::IsEnabled())
    {
        InfoWindow.Reset();
        GPUProfilerWindow.Reset();
    }
}

bool FSceneRenderer::Initialize()
{ 
    Resources.CurrentRenderWidth  = RenderSettings::GetRenderWidth();
    Resources.CurrentRenderHeight = RenderSettings::GetRenderHeight();

    if (RenderSettings::NeedsResize())
    {
        RenderSettings::OnDidChangeRenderResolution(Resources.CurrentRenderWidth, Resources.CurrentRenderHeight);
    }

    FRHIBufferInfo CBInfo;
    CBInfo.Size   = sizeof(FCameraHLSL);
    CBInfo.Stride = sizeof(FCameraHLSL);
    CBInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
    CBInfo.bEnableResourceStateTracking = true;

    Resources.CameraBuffer = FRHI::Get()->CreateBuffer(CBInfo, EResourceAccess::Common, nullptr);
    if (!Resources.CameraBuffer)
    {
        LOG_ERROR("[Renderer]: Failed to create CameraBuffer");
        return false;
    }
    else
    {
        Resources.CameraBuffer->SetDebugName("CameraBuffer");
    }

    // Initialize standard input layout
    TArray<FRHIInputElementInfo> InputElements =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0,  0, EVertexInputClass::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, sizeof(FVertexNormal),   1, 0,  1, EVertexInputClass::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, sizeof(FVertexNormal),   1, 12, 2, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 2, 0,  3, EVertexInputClass::Vertex, 0 },
    };

    Resources.MeshInputLayout = FRHI::Get()->CreateInputLayout(InputElements);
    if (!Resources.MeshInputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FRHISamplerStateInfo SamplerStateInfo;
        SamplerStateInfo.AddressU       = ESamplerMode::Clamp;
        SamplerStateInfo.AddressV       = ESamplerMode::Clamp;
        SamplerStateInfo.AddressW       = ESamplerMode::Clamp;
        SamplerStateInfo.Filter         = ESamplerFilter::MinMagMipPoint;
        SamplerStateInfo.ComparisonFunc = EComparisonFunc::Unknown;
        SamplerStateInfo.MinLOD         = 0.0f;
        SamplerStateInfo.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        Resources.ShadowSamplerPoint = FRHI::Get()->CreateSamplerState(SamplerStateInfo);
        if (!Resources.ShadowSamplerPoint)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    {
        FRHISamplerStateInfo SamplerStateInfo;
        SamplerStateInfo.AddressU       = ESamplerMode::Clamp;
        SamplerStateInfo.AddressV       = ESamplerMode::Clamp;
        SamplerStateInfo.AddressW       = ESamplerMode::Clamp;
        SamplerStateInfo.Filter         = ESamplerFilter::Comparison_MinMagMipPoint;
        SamplerStateInfo.ComparisonFunc = EComparisonFunc::LessEqual;
        SamplerStateInfo.MinLOD         = 0.0f;
        SamplerStateInfo.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        Resources.ShadowSamplerPointCmp = FRHI::Get()->CreateSamplerState(SamplerStateInfo);
        if (!Resources.ShadowSamplerPointCmp)
        {
            DEBUG_BREAK();
            return false;
        }

        SamplerStateInfo.Filter = ESamplerFilter::Comparison_MinMagMipLinear;

        Resources.ShadowSamplerLinearCmp = FRHI::Get()->CreateSamplerState(SamplerStateInfo);
        if (!Resources.ShadowSamplerLinearCmp)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    {
        FRHISamplerStateInfo SamplerStateInfo;
        SamplerStateInfo.AddressU       = ESamplerMode::Wrap;
        SamplerStateInfo.AddressV       = ESamplerMode::Wrap;
        SamplerStateInfo.AddressW       = ESamplerMode::Wrap;
        SamplerStateInfo.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        SamplerStateInfo.ComparisonFunc = EComparisonFunc::LessEqual;
        SamplerStateInfo.MinLOD         = 0.0f;
        SamplerStateInfo.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        Resources.PointLightShadowSampler = FRHI::Get()->CreateSamplerState(SamplerStateInfo);
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

    if (false/*RHIDeviceFeatureSupport::bSupportsRayTracing*/)
    {
        if (!RayTracer.Initialize(Resources))
        {
            return false;
        }
    }

    // Register ImGui Windows
    if (IImguiPlugin::IsEnabled())
    {
        InfoWindow        = MakeSharedPtr<FRendererInfoWidget>(this);
        GPUProfilerWindow = MakeSharedPtr<FGPUProfilerWidget>();
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

    ShadowMaskHistoryPass = new FShadowMaskHistoryPass(this);
    if (!ShadowMaskHistoryPass->Initialize(Resources))
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

    DebugViewPass = new FDebugViewPass(this);
    if (!DebugViewPass->Initialize(Resources))
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

void FSceneRenderer::BeginFrame()
{
    FRHICommandListExecutor::Get().Tick();

    // Clear the images that were "debug-able" last frame 
    // Update FrameCounter
    FrameCounter.NextFrame();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "-------- Begin Frame --------");

    CommandList.BeginFrame();
    
    // Resize SwapChains before doing anything else
    {
        TRACE_SCOPE("Resize SwapChains");

        for (const FSwapChainResizeInfo& ResizeInfo : SwapChainsToResize)
        {
            CommandList.ResizeSwapChain(ResizeInfo.SwapChain.Get(), ResizeInfo.Width, ResizeInfo.Height);
        }

        SwapChainsToResize.Clear();
    }    
    
    CommandList.BeginExternalCapture();

    // Begin capture GPU FrameTime
    FGPUProfiler::Get().BeginGPUFrame(CommandList);

    // Prepare SwapChains by transition the back-buffers to the correct resource state
    {
        TRACE_SCOPE("Prepare SwapChains");

        for (FRHISwapChainRef SwapChain : SwapChainsToPrepare)
        {
            FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
            CommandList.RequireTextureState(BackBuffer, FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
        }

        SwapChainsToPrepare.Clear();
    }
}

void FSceneRenderer::RenderSceneView(const FSceneRenderView& SceneRenderView)
{
    // Ensure render resolution matches the actual render target size
    if (SceneRenderView.RenderTarget)
    {
        const uint32 TargetWidth  = SceneRenderView.RenderTarget->GetWidth();
        const uint32 TargetHeight = SceneRenderView.RenderTarget->GetHeight();

        RenderSettings::ChangeRenderResolution(TargetWidth, TargetHeight);
    }

    // Resize the renderer resources
    if (RenderSettings::NeedsResize())
    {
        ResizeResources(RenderSettings::GetRenderWidth(), RenderSettings::GetRenderHeight());
    }

    // Cast and cache the current scene being rendered
    FScene* CurrentScene = static_cast<FScene*>(SceneRenderView.Scene);
    
    // Prepare Lights
    Resources.BuildLightBuffers(CommandList, CurrentScene);

    // Recreate cascade resources when the cascade size changes.
    if (Resources.bCascadeSizeDirty)
    {
        if (!CascadedShadowsRenderPass || !CascadedShadowsRenderPass->CreateResources(Resources))
        {
            LOG_ERROR("[Renderer]: Failed to recreate shadow cascade resources");
            return;
        }

        Resources.bCascadeSizeDirty = false;
    }

    // Update camera-buffer
    FCamera* Camera = CurrentScene->Camera;

    CameraBuffer.PrevViewProjection          = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjection              = Camera->GetViewProjectionMatrix();
    CameraBuffer.ViewProjectionInv           = Camera->GetViewProjectionInverseMatrix();
    CameraBuffer.ViewProjectionUnjittered    = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjectionInvUnjittered = CameraBuffer.ViewProjectionInv;
    CameraBuffer.View                        = Camera->GetViewMatrix();
    CameraBuffer.ViewInv                     = Camera->GetViewInverseMatrix();
    CameraBuffer.Projection                  = Camera->GetProjectionMatrix();
    CameraBuffer.ProjectionInv               = Camera->GetProjectionInverseMatrix();
    CameraBuffer.ProjectionUnjittered        = CameraBuffer.Projection;
    CameraBuffer.ProjectionInvUnjittered     = CameraBuffer.ProjectionInv;
    CameraBuffer.Position                    = Camera->GetPosition();
    CameraBuffer.Forward                     = Camera->GetForwardVector();
    CameraBuffer.Right                       = Camera->GetRightVector();
    CameraBuffer.NearPlane                   = Camera->GetNearPlane();
    CameraBuffer.FarPlane                    = Camera->GetFarPlane();
    CameraBuffer.AspectRatio                 = Camera->GetAspectRatio();
    CameraBuffer.ViewportWidth               = float(Resources.CurrentRenderWidth);
    CameraBuffer.ViewportHeight              = float(Resources.CurrentRenderHeight);

    if (CVarEnableTemporalAA.GetValue())
    {
        const FVector2 CameraJitter    = HaltonState.NextSample();
        const FVector2 ClipSpaceJitter = CameraJitter / FVector2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

        // Add Jitter to projection matrix
        FMatrix4 JitterOffset      = FMatrix4::Translation(FVector3(ClipSpaceJitter.X, ClipSpaceJitter.Y, 0.0f));
        CameraBuffer.Projection    = CameraBuffer.Projection * JitterOffset;
        CameraBuffer.ProjectionInv = CameraBuffer.Projection.GetInverse();

        // Calculate new ViewProjection
        CameraBuffer.ViewProjection    = CameraBuffer.View * CameraBuffer.Projection;
        CameraBuffer.ViewProjectionInv = CameraBuffer.ViewProjection.GetInverse();
        CameraBuffer.PrevJitter        = CameraBuffer.Jitter;
        CameraBuffer.Jitter            = ClipSpaceJitter;
    }
    else
    {
        CameraBuffer.PrevJitter = FVector2(0.0f);
        CameraBuffer.Jitter     = FVector2(0.0f);
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

    // Update GPU Camera Buffer
    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(Resources.CameraBuffer.Get(), FBufferRegion(0, sizeof(FCameraHLSL)), &CameraBuffer);
    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    // Compile material PSOs
    for (FMaterial* Material : CurrentScene->Materials)
    {
        DepthPrePass->InitializePipelineState(Material, Resources);

    #if EDITOR_BUILD
        EditorNoJitterDepthPass->InitializePipelineState(Material, Resources);
        EditorSelectionIDPass->InitializePipelineState(Material, Resources);
    #endif

        BasePass->InitializePipelineState(Material, Resources);
        PointLightRenderPass->InitializePipelineState(Material, Resources);
        CascadedShadowsRenderPass->InitializePipelineState(Material, Resources);

        if (Material->IsBufferDirty())
        {
            Material->BuildBuffer(CommandList);
        }
    }

    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Albedo].Get(), FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Normal].Get(), FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Material].Get(), FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Velocity].Get(), FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHIRequiredTextureState::Make(EResourceAccess::DepthWrite));

    // PrePass
    if (CVarPrePassEnabled.GetValue())
    {
        DepthPrePass->Execute(CommandList, Resources, CurrentScene);
    }
    else
    {
        FRHIDepthStencilView DepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get());
        CommandList.ClearDepthStencilView(DepthStencilView, 1.0f, 0);
    }

#if SUPPORT_VARIABLE_RATE_SHADING
    if (ShadingImage && CVarEnableVariableRateShading.GetValue())
    {
        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin VRS Image");
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);

        CommandList.RequireTextureState(ShadingImage.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));

        CommandList.SetComputePipelineState(ShadingRatePipeline.Get());

        FRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        CommandList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

        CommandList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);

        CommandList.RequireTextureState(ShadingImage.Get(), FRHIRequiredTextureState::Make(EResourceAccess::ShadingRateSource));

        CommandList.SetShadingRateImage(ShadingImage.Get());

        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End VRS Image");
    }
    else if (RHISupportsVariableRateShading())
    {
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);
    }
#endif

    // BasePass
    if (CVarBasePassEnabled.GetValue())
    {
        BasePass->Execute(CommandList, Resources, CurrentScene);
    }

    const bool bEnableShadows    = CVarShadowsEnabled.GetValue();
    const bool bEnableSunShadows = CVarSunShadowsEnabled.GetValue();

    // Depth Reduce
    bool bAdaptiveSplitRange = false;
    if (IConsoleVariable* CVarAdaptiveSplitRange = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.AdaptiveSplitRange"))
    {
        bAdaptiveSplitRange = CVarAdaptiveSplitRange->GetBool();
    }

    if ((CVarCSMTightFrustum.GetValue() || bAdaptiveSplitRange) && bEnableShadows && bEnableSunShadows)
    {
        DepthReducePass->Execute(CommandList, Resources, CurrentScene);
    }

    // RayTracing PrePass
    if (false /*RHIDeviceFeatureSupport::bSupportsRayTracing*/)
    {
        GPU_TRACE_SCOPE(CommandList, "Ray Tracing");
        RayTracer.PreRender(CommandList, Resources, CurrentScene);
    }

    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Albedo].Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Normal].Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Velocity].Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Material].Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
    CommandList.RequireTextureState(Resources.SSAOBuffer.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));

    // SSAO
    if (CVarEnableSSAO.GetValue())
    {
        ScreenSpaceOcclusionPass->Execute(CommandList, Resources);
    }
    else
    {
        CommandList.ClearUnorderedAccessViewFloat(Resources.SSAOBuffer->GetUnorderedAccessView(), FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    CommandList.RequireTextureState(Resources.SSAOBuffer.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));

    // Render Shadows
    if (bEnableShadows)
    {
        // Point Lights
        if (CVarPointLightShadowsEnabled.GetValue())
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
    CommandList.RequireTextureState(Resources.FinalTarget.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));
    CommandList.RequireTextureState(Resources.IntegrationLUT.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));

    if (CurrentScene)
    {
        if (FSceneSkyLight* SkyLight = CurrentScene->SkyLight)
        {
            CommandList.RequireTextureState(SkyLight->DiffuseCubeMap.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
            CommandList.RequireTextureState(SkyLight->SpecularCubeMap.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
        }
    }

    // In order to render the shadow-mask, we want all these features to be enabled
    const bool bEnableShadowMask = CVarShadowMaskEnabled.GetValue();
    if (bEnableShadows && bEnableShadowMask && bEnableSunShadows)
    {
        const auto ResolveShadowDebugMode = [](FSceneRenderView::EDebugView InView) -> EShadowDebugMode
        {
            switch (InView)
            {
            case FSceneRenderView::EDebugView::ShadowCascadeIndex:      return EShadowDebugMode::CascadeIndex;
            case FSceneRenderView::EDebugView::ShadowCascadeTransition: return EShadowDebugMode::CascadeTransition;
            case FSceneRenderView::EDebugView::ShadowFilterMargin:      return EShadowDebugMode::FilterMargin;
            case FSceneRenderView::EDebugView::ShadowPCSSRadiusClamp:   return EShadowDebugMode::PCSSRadiusClamp;
            case FSceneRenderView::EDebugView::ShadowCascadeUpdated:    return EShadowDebugMode::CascadeUpdated;
            case FSceneRenderView::EDebugView::ShadowContainment:       return EShadowDebugMode::Containment;
            case FSceneRenderView::EDebugView::ShadowCascadeFallback:   return EShadowDebugMode::CascadeFallback;
            default:                                                    return EShadowDebugMode::None;
            }
        };

        EShadowDebugMode ShadowDebug = ResolveShadowDebugMode(SceneRenderView.DebugView);
        if (ShadowDebug == EShadowDebugMode::None)
        {
            ShadowDebug = ResolveShadowDebugMode(SceneRenderView.SecondaryDebugView);
        }


        bool bUseShadowHistory = false;
        if (IConsoleVariable* CVarShadowHistory = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.ShadowHistory"))
        {
            bUseShadowHistory = CVarShadowHistory->GetBool();
        }

        if (!bUseShadowHistory)
        {
            Resources.bShadowMaskHistoryInitialized = false;
        }

        const uint32 ShadowDebugMode = static_cast<uint32>(ShadowDebug);
        ShadowMaskRenderPass->Execute(CommandList, Resources, ShadowDebugMode, bUseShadowHistory);

        if (bUseShadowHistory)
        {
            ShadowMaskHistoryPass->Execute(CommandList, Resources);
        }
    }
    else
    {
        CommandList.RequireTextureState(Resources.DirectionalShadowMask.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));
        CommandList.RequireTextureState(Resources.CascadeIndexBuffer.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));

        Resources.bShadowMaskHistoryInitialized = false;

        const FVector4 MaskClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        CommandList.ClearUnorderedAccessViewFloat(Resources.DirectionalShadowMask->GetUnorderedAccessView(), MaskClearColor);

        const FVector4 DebugClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.ClearUnorderedAccessViewFloat(Resources.CascadeIndexBuffer->GetUnorderedAccessView(), DebugClearColor);

        if (Resources.ShadowDebugBuffer)
        {
            CommandList.RequireTextureState(Resources.ShadowDebugBuffer.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));
            CommandList.ClearUnorderedAccessViewFloat(Resources.ShadowDebugBuffer->GetUnorderedAccessView(), FVector4(0.0f, 0.0f, 0.0f, 0.0f));
            CommandList.RequireTextureState(Resources.ShadowDebugBuffer.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
        }

        CommandList.RequireTextureState(Resources.CascadeIndexBuffer.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
        CommandList.RequireTextureState(Resources.DirectionalShadowMask.Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
    }

    const bool bDrawCascadesOverlayMain = (SceneRenderView.DebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay);
    const bool bDrawCascadesOverlayAny  = bDrawCascadesOverlayMain || (SceneRenderView.SecondaryDebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay);

    if (IConsoleVariable* CVarDebugData = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.DebugData"))
    {
        CVarDebugData->SetAsBool(bDrawCascadesOverlayAny, EConsoleVariableFlags::SetByCode);
    }

    if (bDrawCascadesOverlayMain)
    {
        if (IConsoleVariable* CVarDrawCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawCascades"))
        {
            CVarDrawCascades->SetAsBool(false, EConsoleVariableFlags::SetByCode);
        }
    }

    // Main LightPass
    TiledLightPass->Execute(CommandList, Resources, CurrentScene);

    CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHIRequiredTextureState::Make(EResourceAccess::DepthWrite));
    CommandList.RequireTextureState(Resources.FinalTarget.Get(), FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));

    // Skybox Pass
    if (CVarSkyboxEnabled.GetValue())
    {
        SkyboxRenderPass->Execute(CommandList, Resources, CurrentScene);
    }

    CommandList.RequireTextureState(Resources.PointLightShadowMaps.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));

    if (CurrentScene)
    {
        if (FSceneSkyLight* SkyLight = CurrentScene->SkyLight)
        {
            CommandList.RequireTextureState(SkyLight->DiffuseCubeMap.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
            CommandList.RequireTextureState(SkyLight->SpecularCubeMap.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
        }
    }

    CommandList.RequireTextureState(Resources.IntegrationLUT.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));

    // Forward Pass
    // if (!Resources.ForwardVisibleCommands.IsEmpty())
    // {
        // ForwardPass->Execute(CommandList, Resources, Scene);
    // }

    // Debug PointLights
    if (CVarDrawPointLights.GetValue())
    {
        DebugRenderer->RenderPointLights(CommandList, Resources, CurrentScene);
    }

    // Debug LightProbes
    if (CVarDrawLightProbes.GetValue())
    {
        DebugRenderer->RenderLightProbes(CommandList, Resources, CurrentScene);
    }

    // Debug AABBs
    if (CVarDrawAABBs.GetValue())
    {
        DebugRenderer->RenderObjectAABBs(CommandList, Resources, CurrentScene);
    }

    // Temporal AA
    if (CVarEnableTemporalAA.GetValue())
    {
        CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
        CommandList.RequireTextureState(Resources.FinalTarget.Get(), FRHIRequiredTextureState::Make(EResourceAccess::UnorderedAccess));

        TemporalAA->Execute(CommandList, Resources);

        CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
        CommandList.RequireTextureState(Resources.FinalTarget.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
    }
    else
    {
        CommandList.RequireTextureState(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
        CommandList.RequireTextureState(Resources.FinalTarget.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
    }

    // Editor selection outline (ObjectID -> mask -> dilate/erode -> ring -> composite after tonemap)
#if EDITOR_BUILD
    {
        TArray<uint32> SelectedObjectIDs;

        if (FEditorEngine* EditorEngine = static_cast<FEditorEngine*>(FEngine::Get()))
        {
            if (FActor* SelectedActor = EditorEngine->GetSelectedActor())
            {
                SelectedObjectIDs.Add(CurrentScene->GetOrCreateObjectID(SelectedActor));
            }
        }

        EditorNoJitterDepthPass->Execute(CommandList, Resources, CurrentScene);
        EditorSelectionIDPass->Execute(CommandList, Resources, CurrentScene); 
 
        ProcessEditorObjectPickRequests(CommandList, Resources, CurrentScene); 
 
        SelectionOutlinePass->Execute(CommandList, Resources, SelectedObjectIDs); 
    } 
#endif 

    if (IConsoleVariable* CVarDrawTileDebug = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawTiledLightning"))
    {
        const bool bTileDebug = (SceneRenderView.DebugView == FSceneRenderView::EDebugView::TileOccupancy);
        CVarDrawTileDebug->SetAsBool(bTileDebug, EConsoleVariableFlags::SetByCode);
    }

    const bool bMainUsesDebugPass =
        SceneRenderView.DebugView != FSceneRenderView::EDebugView::None &&
        SceneRenderView.DebugView != FSceneRenderView::EDebugView::ShadowCascadeOverlay &&
        SceneRenderView.DebugView != FSceneRenderView::EDebugView::TileOccupancy;

    const bool bMainUsesOverlay   = SceneRenderView.DebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay;
    const bool bHasSecondaryDebug = SceneRenderView.SecondaryDebugView != FSceneRenderView::EDebugView::None;
    const bool bUsesDebugOutput   = bMainUsesDebugPass || bMainUsesOverlay;

    FSceneRenderView DebugViewRender = SceneRenderView;

#if EDITOR_BUILD
    FRHITexture* DebugOutputTarget = nullptr;
    if (bUsesDebugOutput && Resources.FinalTarget)
    {
        DebugOutputTarget            = Resources.FinalTarget.Get();
        DebugViewRender.RenderTarget = DebugOutputTarget;
    }
#endif

    // FXAA
    if (CVarEnableFXAA.GetValue())
    {
        FXAAPass->Execute(CommandList, SceneRenderView, Resources);
    }

    // Perform ToneMapping and output to BackBuffer
#if EDITOR_BUILD
    TonemapPass->Execute(CommandList, Resources, Resources.TonemappedTarget.Get(), false);
#else
    TonemapPass->Execute(CommandList, Resources, SceneRenderView.RenderTarget, true);
#endif

    if (bMainUsesDebugPass)
    {
        DebugViewPass->Execute(CommandList, DebugViewRender, Resources, SceneRenderView.DebugView);
    }

    if (bMainUsesOverlay)
    {
        FRHITexture* RenderTarget = DebugViewRender.RenderTarget;
        if (RenderTarget)
        {
            const int32 TargetWidth  = static_cast<int32>(RenderTarget->GetWidth());
            const int32 TargetHeight = static_cast<int32>(RenderTarget->GetHeight());

            DebugViewPass->ExecuteOverlay(CommandList, DebugViewRender, Resources, SceneRenderView.DebugView, 0, 0, TargetWidth, TargetHeight);
        }
    }

    // Composite the grid and selection outline
#if EDITOR_BUILD
    FRHITexture* CompositeInput = DebugOutputTarget ? DebugOutputTarget : Resources.TonemappedTarget.Get();
    FinalCompositePass->Execute(CommandList, SceneRenderView, Resources, CompositeInput);
#endif

    if (bHasSecondaryDebug)
    {
        FRHITexture* RenderTarget = SceneRenderView.RenderTarget;
        if (RenderTarget)
        {
            const int32 TargetWidth   = static_cast<int32>(RenderTarget->GetWidth());
            const int32 TargetHeight  = static_cast<int32>(RenderTarget->GetHeight());
            const int32 OverlayWidth  = Math::Max(TargetWidth / 2, 1);
            const int32 OverlayHeight = Math::Max(TargetHeight / 2, 1);
            const int32 OverlayX      = TargetWidth - OverlayWidth;
            const int32 OverlayY      = 0;

            DebugViewPass->ExecuteOverlay(CommandList, SceneRenderView, Resources, SceneRenderView.SecondaryDebugView, OverlayX, OverlayY, OverlayWidth, OverlayHeight);
        }
    }
} 
 
#if EDITOR_BUILD
void FSceneRenderer::ProcessEditorObjectPickRequests(FRHICommandList& InCommandList, FFrameResources& InResources, FScene* CurrentScene)
{
    // Optional editor picking: copy a small region from the ObjectID render target to a readback buffer and signal a fence.
    if (InFlightObjectPicks.Size() >= MaxInFlightObjectPicks)
    {
        return;
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

    const uint32 TexWidth  = InResources.EditorObjectID_NoJitter->GetWidth();
    const uint32 TexHeight = InResources.EditorObjectID_NoJitter->GetHeight();

    if (TexWidth == 0 || TexHeight == 0)
    {
        return;
    }

    const uint32 PixelX        = Math::Min(Request.PixelX, TexWidth - 1);
    const uint32 PixelY        = Math::Min(Request.PixelY, TexHeight - 1);
    const uint32 BytesPerPixel = GetByteStrideFromFormat(InResources.EditorObjectID_NoJitter->GetFormat());

    if (BytesPerPixel == 0)
    {
        // Unsupported format; skip pick.
        return;
    }

    const int32 RequestedRadius = CVarEditorPickSearchRadius.GetValue();
    const int32 SampleRadius    = Math::Clamp(RequestedRadius, 0, 64);
    const bool  bTryFlipY       = CVarEditorPickTryFlipY.GetValue();

    const int32 X0 = Math::Clamp<int32>(int32(PixelX) - SampleRadius, 0, int32(TexWidth) - 1);
    const int32 X1 = Math::Clamp<int32>(int32(PixelX) + SampleRadius, 0, int32(TexWidth) - 1);
    const int32 Y0 = Math::Clamp<int32>(int32(PixelY) - SampleRadius, 0, int32(TexHeight) - 1);
    const int32 Y1 = Math::Clamp<int32>(int32(PixelY) + SampleRadius, 0, int32(TexHeight) - 1);

    const uint32 RegionWidth  = uint32((X1 - X0) + 1);
    const uint32 RegionHeight = uint32((Y1 - Y0) + 1);
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

    FRHIBufferInfo ReadbackInfo;
    ReadbackInfo.Flags  = EBufferFlags::ReadBack;
    ReadbackInfo.Stride = BytesPerPixel;
    ReadbackInfo.Size   = bTryFlipY ? (FlippedBaseOffset + FlippedRequiredSize) : NormalRequiredSize;
    ReadbackInfo.bEnableResourceStateTracking = true;

    FRHIBufferRef   ReadbackBuffer = FRHI::Get()->CreateBuffer(ReadbackInfo, EResourceAccess::CopyDest, nullptr);
    FRHIGpuFenceRef Fence          = FRHI::Get()->CreateFence();

    if (!(ReadbackBuffer && Fence))
    {
        return;
    }

    ReadbackBuffer->SetDebugName("EditorObjectPick Readback");
    Fence->SetDebugName("EditorObjectPick Fence");

    if (CVarEditorPickDebug.GetValue())
    {
        LOG_INFO("[EditorPick] Request. Pixel=(%u,%u) Tex=%ux%u Radius=%d TryFlipY=%s Region=(%u,%u) CenterLocal=(%u,%u)",
            PixelX, PixelY, TexWidth, TexHeight, SampleRadius, bTryFlipY ? "true" : "false",
            RegionWidth, RegionHeight, CenterLocalX, CenterLocalY);
    }

    InCommandList.RequireTextureState(InResources.EditorObjectID_NoJitter.Get(), FRHIRequiredTextureState::Make(EResourceAccess::CopySource));

    // Copy a rectangular neighborhood into a readback buffer.
    // We do one copy per row to control destination row stride across backends and keep D3D12 offsets 512-byte aligned.

    for (uint32 Row = 0; Row < RegionHeight; ++Row)
    {
        const uint64 DstOffset = NormalRowStrideBytes * uint64(Row);
        InCommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), DstOffset, InResources.EditorObjectID_NoJitter.Get(), FTextureRegion2D(RegionWidth, 1, uint32(X0), uint32(Y0) + Row), 0);
    }

    if (bTryFlipY)
    {
        for (uint32 Row = 0; Row < FlippedRegionHeight; ++Row)
        {
            const uint64 DstOffset = FlippedBaseOffset + (FlippedRowStrideBytes * uint64(Row));
            InCommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), DstOffset, InResources.EditorObjectID_NoJitter.Get(), FTextureRegion2D(FlippedRegionWidth, 1, uint32(FlipX0), uint32(FlipY0) + Row), 0);
        }
    }

    InCommandList.RequireTextureState(InResources.EditorObjectID_NoJitter.Get(), FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
    InCommandList.WriteFence(Fence.Get());

    FEditorObjectPickInFlight InFlight;
    InFlight.Scene                 = CurrentScene;
    InFlight.ReadbackBuffer        = ReadbackBuffer;
    InFlight.Fence                 = Fence;
    InFlight.SampleRadius          = uint32(SampleRadius);
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

    InFlightObjectPicks.Add(Move(InFlight));
}
#endif

void FSceneRenderer::RequestEditorObjectPick(FScene* Scene, uint32 PixelX, uint32 PixelY)  
{  
#if EDITOR_BUILD 
    if (Scene)  
    { 
        PendingObjectPicks.Enqueue(FEditorObjectPickRequest{ Scene, PixelX, PixelY }); 
    } 
#else
    UNREFERENCED_VARIABLE(Scene);
    UNREFERENCED_VARIABLE(PixelX);
    UNREFERENCED_VARIABLE(PixelY);
#endif
} 
 
bool FSceneRenderer::PollEditorObjectPickResult(FScene* Scene, uint32& OutObjectID) 
{ 
#if EDITOR_BUILD
    if (!Scene) 
    { 
        return false; 
    } 

    for (int32 Index = 0; Index < InFlightObjectPicks.Size(); ++Index)
    {
        const FEditorObjectPickInFlight& InFlight = InFlightObjectPicks[Index];
        if (InFlight.Scene != Scene)
        {
            continue;
        }

        if (InFlight.Fence && InFlight.Fence->IsSignaled() && InFlight.ReadbackBuffer)
        {
            const uint32 BytesPerPixel = InFlight.ReadbackBuffer->GetInfo().Stride ? InFlight.ReadbackBuffer->GetInfo().Stride : sizeof(uint32);
            const uint64 BufferSize    = InFlight.ReadbackBuffer->GetInfo().Size;

            uint32 ObjectID = 0;
            if (BytesPerPixel >= sizeof(uint32) && BufferSize >= sizeof(uint32))
            {
                if (void* Data = InFlight.ReadbackBuffer->Map(0, BufferSize))
                {
                    const uint8* Base = reinterpret_cast<const uint8*>(Data);

                    const auto ReadPixel = [&](uint32 WindowBaseOffsetBytes, uint32 RowPitch, uint32 Width, uint32 Height, uint32 X, uint32 Y) -> uint32
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

                    const auto ChooseFromWindow = [&](uint32 WindowBaseOffsetBytes, uint32 RowPitch, uint32 Width, uint32 Height, uint32 CenterX, uint32 CenterY) -> uint32
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

                                    const int32 Dx    = int32(X) - int32(CenterX);
                                    const int32 Dy    = int32(Y) - int32(CenterY);
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

                    if (CVarEditorPickDebug.GetValue())
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

                        const auto ComputeStats = [&](uint32 BaseOffset, uint32 RowPitch, uint32 Width, uint32 Height, uint32& OutNonZero)
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

            OutObjectID = ObjectID;
            InFlightObjectPicks.RemoveAtSwap(Index);
            return true;
        }
    }
 
    return false; 
#else
    UNREFERENCED_VARIABLE(Scene);
    OutObjectID = 0;
    return false;
#endif
} 

void FSceneRenderer::RenderUI()
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "-------- Begin UI Render --------");

    {
        TRACE_SCOPE("Render UI");

    #if SUPPORT_VARIABLE_RATE_SHADING
        if (RHISupportsVariableRateShading())
        {
            CommandList.SetShadingRate(EShadingRate::VRS_1x1);
            CommandList.SetShadingRateImage(nullptr);
        }
    #endif

        if (IImguiPlugin::IsEnabled())
        {
            IImguiPlugin::Get().Draw(CommandList);
        }
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "-------- End UI Render --------");
}

void FSceneRenderer::EndFrame()
{
    FGPUProfiler::Get().EndGPUFrame(CommandList);

    CommandList.EndExternalCapture();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "-------- End Frame --------");

    {
        TRACE_SCOPE("Present SwapChains");

        const bool bEnableVSync = CVarVSyncEnabled.GetValue();
        for (FRHISwapChainRef SwapChain : SwapChainsToPresent)
        {
            FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
            CommandList.RequireTextureState(BackBuffer, FRHIRequiredTextureState::Make(EResourceAccess::Present));
            CommandList.PresentSwapChain(SwapChain.Get(), bEnableVSync);
        }

        SwapChainsToPresent.Clear();
    }
    
    CommandList.EndFrame();

    CommandList.FlushDeletedResources();

    {
        TRACE_SCOPE("ExecuteCommandList");

        // Wait for the last frame to finish on the RHI thread
        if (LastFrameFinishedEvent)
        {
            LastFrameFinishedEvent->Wait(FTimespan::Infinity());
            FPlatformEvent::Recycle(LastFrameFinishedEvent);
            LastFrameFinishedEvent = nullptr;
        }

        LastFrameFinishedEvent = FPlatformEvent::Create(false);
        if (LastFrameFinishedEvent)
        {
            CommandList.SetEvent(LastFrameFinishedEvent);
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }
}

void FSceneRenderer::ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 InWidth, uint32 InHeight)
{
    if (SwapChain)
    {
        SwapChainsToResize.Emplace(SwapChain, InWidth, InHeight);
    }
}

void FSceneRenderer::ResizeResources(uint32 InWidth, uint32 InHeight)
{
    if ((Resources.CurrentRenderWidth != InWidth || Resources.CurrentRenderHeight != InHeight) && InWidth > 0 && InHeight > 0)
    {
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

    #if EDITOR_BUILD
        if (!SelectionOutlinePass->CreateResources(InWidth, InHeight))
        {
            DEBUG_BREAK();
            return;
        }
    #endif

        LOG_INFO("Changed render-resolution. From: w=%d h=%d, To: w=%d h=%d", Resources.CurrentRenderWidth, Resources.CurrentRenderHeight, InWidth, InHeight);

        Resources.CurrentRenderWidth  = InWidth;
        Resources.CurrentRenderHeight = InHeight;

        RenderSettings::OnDidChangeRenderResolution(InWidth, InHeight);
    }
}

void FSceneRenderer::PrepareSwapChain(FRHISwapChainRef SwapChain)
{
    if (SwapChain)
    {
        SwapChainsToPrepare.Add(SwapChain);
    }
}

void FSceneRenderer::PresentSwapChain(FRHISwapChainRef SwapChain)
{
    if (SwapChain)
    {
        SwapChainsToPresent.Add(SwapChain);
    }
}

bool FSceneRenderer::InitShadingImage()
{
    if (RHIDeviceFeatureSupport::ShadingRateTier != EShadingRateTier::Tier2 || RHIDeviceFeatureSupport::ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.CurrentRenderWidth / RHIDeviceFeatureSupport::ShadingRateImageTileSize;
    const uint32 Height = Resources.CurrentRenderHeight / RHIDeviceFeatureSupport::ShadingRateImageTileSize;

    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::ShadingRateTexture;
    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, UsageFlags);
    TextureInfo.bEnableResourceStateTracking = true;

    ShadingImage = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::ShadingRateSource);
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

    ShadingRateShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!ShadingRateShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInfo PSOInfo;
    PSOInfo.Shader = ShadingRateShader.Get();

    ShadingRatePipeline = FRHI::Get()->CreateComputePipelineState(PSOInfo);
    if (!ShadingRatePipeline)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}
