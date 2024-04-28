#include "SceneRenderer.h"
#include "Debug/GPUProfiler.h"
#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Time/Timespan.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Application/Application.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Engine.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "RendererCore/TextureFactory.h"

#define SUPPORT_VARIABLE_RATE_SHADING (0)

static TAutoConsoleVariable<bool> CVarEnableSSAO(
    "Renderer.Feature.SSAO",
    "Enables Screen-Space Ambient Occlusion",
    true);

static TAutoConsoleVariable<bool> CVarEnableFXAA(
    "Renderer.Feature.FXAA",
    "Enables FXAA for Anti-Aliasing",
    false);

static TAutoConsoleVariable<bool> CVarEnableTemporalAA(
    "Renderer.Feature.TemporalAA",
    "Enables Temporal Anti-Aliasing",
    true);

static TAutoConsoleVariable<bool> CVarEnableVariableRateShading(
    "Renderer.Feature.VariableRateShading",
    "Enables VRS (Variable Rate Shading)",
    false);

static TAutoConsoleVariable<bool> CVarPrePassEnabled(
    "Renderer.Feature.PrePass",
    "Enables Pre-Pass",
    true);

static TAutoConsoleVariable<bool> CVarBasePassEnabled(
    "Renderer.Feature.BasePass",
    "Enables BasePass (Disabling this disables most rendering)",
    true);

static TAutoConsoleVariable<bool> CVarShadowsEnabled(
    "Renderer.Feature.Shadows",
    "Enables Rendering of ShadowMaps",
    true);

static TAutoConsoleVariable<bool> CVarShadowMaskEnabled(
    "Renderer.Feature.ShadowMask",
    "Enables Rendering of ShadowMask for SunShadows",
    true);

static TAutoConsoleVariable<bool> CVarPointLightShadowsEnabled(
    "Renderer.Feature.PointLightShadows",
    "Enables Rendering of PointLight ShadowMaps",
    true);

static TAutoConsoleVariable<bool> CVarSunShadowsEnabled(
    "Renderer.Feature.SunShadows",
    "Enables Rendering of SunLight/DirectionalLight ShadowMaps",
    true);

static TAutoConsoleVariable<bool> CVarSkyboxEnabled(
    "Renderer.Feature.Skybox",
    "Enables Rendering of the Skybox",
    true);

static TAutoConsoleVariable<bool> CVarDrawAABBs(
    "Renderer.Debug.DrawAABBs",
    "Draws all the objects bounding boxes (AABB)",
    false);

static TAutoConsoleVariable<bool> CVarDrawPointLights(
    "Renderer.Debug.DrawPointLights", 
    "Draws all the PointLights as spheres with the light-color",
    false);

static TAutoConsoleVariable<bool> CVarVSyncEnabled(
    "Renderer.Feature.VerticalSync",
    "Enables Vertical-Sync", 
    false);

static TAutoConsoleVariable<bool> CVarFrustumCullEnabled(
    "Renderer.Feature.FrustumCulling",
    "Enables Frustum Culling (CPU) for the main scene and for all shadow frustums",
    true);

static TAutoConsoleVariable<bool> CVarRayTracingEnabled(
    "Renderer.Feature.RayTracing",
    "Enables Ray Tracing (Currently broken)",
    false);

static TAutoConsoleVariable<bool> CVarPrePassDepthReduce(
    "Renderer.PrePass.DepthReduce",
    "Set to true to reduce the DepthBuffer to find the Min- and Max Depth in the DepthBuffer",
    true);

FResponse FRendererEventHandler::OnWindowResized(const FWindowEvent& WindowEvent)
{
    if (!GEngine)
    {
        return FResponse::Unhandled();
    }

    if (WindowEvent.GetWindow() != GEngine->MainWindow)
    {
        return FResponse::Unhandled();
    }

    Renderer->ResizeResources(WindowEvent);
    return FResponse::Handled();
}

FSceneRenderer::FSceneRenderer()
    : TextureDebugger(nullptr)
    , InfoWindow(nullptr)
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
    , ScreenSpaceOcclusionPass(nullptr)
    , SkyboxRenderPass(nullptr)
    , LightProbeRenderer(this)
    , TemporalAA(nullptr)
    , ForwardPass(nullptr)
    , TonemapPass(nullptr)
    , RayTracer(this)
    , DebugRenderer(this)
    , ShadingImage(nullptr)
    , ShadingRatePipeline(nullptr)
    , ShadingRateShader(nullptr)
    , TimestampQueries(nullptr)
    , FrameStatistics()
    , LastFrameFinishedEvent(nullptr)
{
}

FSceneRenderer::~FSceneRenderer()
{
    GRHICommandExecutor.WaitForGPU();

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
    SAFE_DELETE(ForwardPass);
    SAFE_DELETE(FXAAPass);
    SAFE_DELETE(TonemapPass);

    LightProbeRenderer.Release();
    RayTracer.Release();
    DebugRenderer.Release();

    Resources.Release();

    ShadingImage.Reset();
    ShadingRatePipeline.Reset();
    ShadingRateShader.Reset();

    TimestampQueries.Reset();

    FrameStatistics.Reset();

    if (FApplication::IsInitialized())
    {
        FApplication::Get().RemoveWidget(TextureDebugger);
        TextureDebugger.Reset();

        FApplication::Get().RemoveWidget(InfoWindow);
        InfoWindow.Reset();

        FApplication::Get().RemoveWidget(GPUProfilerWindow);
        GPUProfilerWindow.Reset();

        FApplication::Get().RemoveEventHandler(EventHandler);
        EventHandler.Reset();
    }
}

bool FSceneRenderer::Initialize()
{
    if (!GEngine->MainViewport)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.MainViewport     = GEngine->MainViewport->GetRHIViewport();
        Resources.BackBufferFormat = Resources.MainViewport->GetColorFormat();
        Resources.CurrentWidth     = Resources.MainViewport->GetWidth();
        Resources.CurrentHeight    = Resources.MainViewport->GetHeight();
    }

    if (FApplication::IsInitialized())
    {
        EventHandler = MakeShared<FRendererEventHandler>(this);
        FApplication::Get().AddEventHandler(EventHandler);
    }
    else
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBufferInfo CBInfo(sizeof(FCameraHLSL), sizeof(FCameraHLSL), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
    Resources.CameraBuffer = RHICreateBuffer(CBInfo, EResourceAccess::Common, nullptr);
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
    FRHIVertexInputLayoutInitializer InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertex), 0, 0,  EVertexInputClass::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, sizeof(FVertex), 0, 12, EVertexInputClass::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, sizeof(FVertex), 0, 24, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertex), 0, 36, EVertexInputClass::Vertex, 0 },
    };

    Resources.MeshInputLayout = RHICreateVertexInputLayout(InputLayout);
    if (!Resources.MeshInputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FRHISamplerStateInfo Initializer;
        Initializer.AddressU    = ESamplerMode::Border;
        Initializer.AddressV    = ESamplerMode::Border;
        Initializer.AddressW    = ESamplerMode::Border;
        Initializer.Filter      = ESamplerFilter::MinMagMipPoint;
        Initializer.BorderColor = FFloatColor(1.0f, 1.0f, 1.0f, 1.0f);

        Resources.DirectionalLightShadowSampler = RHICreateSamplerState(Initializer);
        if (!Resources.DirectionalLightShadowSampler)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    {
        FRHISamplerStateInfo Initializer;
        Initializer.AddressU       = ESamplerMode::Wrap;
        Initializer.AddressV       = ESamplerMode::Wrap;
        Initializer.AddressW       = ESamplerMode::Wrap;
        Initializer.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        Initializer.ComparisonFunc = EComparisonFunc::LessEqual;

        Resources.PointLightShadowSampler = RHICreateSamplerState(Initializer);
        if (!Resources.PointLightShadowSampler)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    if (!InitShadingImage())
        return false;

    if (!DebugRenderer.Initialize(Resources))
        return false;

    if (!Resources.Initialize())
        return false;

    if (!InitializeRenderPasses())
        return false;

    if (!LightProbeRenderer.Initialize(Resources))
        return false;

    if (false/*GRHISupportsRayTracing*/)
    {
        if (!RayTracer.Initialize(Resources))
            return false;
    }

    // Copy over the texture
    {
        LightProbeRenderer.RenderSkyLightProbe(CommandList, Resources);
        
        FRHITextureInfo IrradianceProbeInfo = Resources.Skylight.IrradianceMap->GetInfo();
        IrradianceProbeInfo.UsageFlags = ETextureUsageFlags::ShaderResource;

        FRHITextureInfo SpecularIrradianceProbeInfo = Resources.Skylight.SpecularIrradianceMap->GetInfo();
        SpecularIrradianceProbeInfo.UsageFlags = ETextureUsageFlags::ShaderResource;

        FRHITextureRef IrradianceMap = RHICreateTexture(IrradianceProbeInfo, EResourceAccess::CopyDest);
        if (!IrradianceMap)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            IrradianceMap->SetDebugName("Irradiance Map");
        }

        FRHITextureRef SpecularIrradianceMap = RHICreateTexture(SpecularIrradianceProbeInfo, EResourceAccess::CopyDest);
        if (!SpecularIrradianceMap)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            SpecularIrradianceMap->SetDebugName("Specular Irradiance Map");
        }

        CommandList.TransitionTexture(Resources.Skylight.IrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopySource);
        CommandList.TransitionTexture(Resources.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopySource);

        CommandList.CopyTexture(IrradianceMap.Get(), Resources.Skylight.IrradianceMap.Get());
        CommandList.CopyTexture(SpecularIrradianceMap.Get(), Resources.Skylight.SpecularIrradianceMap.Get());

        CommandList.TransitionTexture(IrradianceMap.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);
        CommandList.TransitionTexture(SpecularIrradianceMap.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        Resources.Skylight.IrradianceMap         = IrradianceMap;
        Resources.Skylight.SpecularIrradianceMap = SpecularIrradianceMap;

        GRHICommandExecutor.ExecuteCommandList(CommandList);
    }

    // Register Windows
    if (FApplication::IsInitialized())
    {
        TextureDebugger = MakeShared<FRenderTargetDebugWindow>();
        FApplication::Get().AddWidget(TextureDebugger);

        InfoWindow = MakeShared<FRendererInfoWindow>(this);
        FApplication::Get().AddWidget(InfoWindow);

        GPUProfilerWindow = MakeShared<FGPUProfilerWindow>();
        FApplication::Get().AddWidget(GPUProfilerWindow);
    }

    return true;
}

bool FSceneRenderer::InitializeRenderPasses()
{
    DepthPrePass = new FDepthPrePass(this);
    if (!DepthPrePass->Initialize(Resources))
        return false;

    BasePass = new FDeferredBasePass(this);
    if (!BasePass->Initialize(Resources))
        return false;

    TiledLightPass = new FTiledLightPass(this);
    if (!TiledLightPass->Initialize(Resources))
        return false;

    DepthReducePass = new FDepthReducePass(this);
    if (!DepthReducePass->Initialize(Resources))
        return false;

    PointLightRenderPass = new FPointLightRenderPass(this);
    if (!PointLightRenderPass->Initialize(Resources))
        return false;

    CascadeGenerationPass = new FCascadeGenerationPass(this);
    if (!CascadeGenerationPass->Initialize(Resources))
        return false;

    CascadedShadowsRenderPass = new FCascadedShadowsRenderPass(this);
    if (!CascadedShadowsRenderPass->Initialize(Resources))
        return false;

    ShadowMaskRenderPass = new FShadowMaskRenderPass(this);
    if (!ShadowMaskRenderPass->Initialize(Resources))
        return false;

    ScreenSpaceOcclusionPass = new FScreenSpaceOcclusionPass(this);
    if (!ScreenSpaceOcclusionPass->Initialize(Resources))
        return false;

    SkyboxRenderPass = new FSkyboxRenderPass(this);
    if (!SkyboxRenderPass->Initialize(Resources))
        return false;

    TemporalAA = new FTemporalAA(this);
    if (!TemporalAA->Initialize(Resources))
        return false;

    ForwardPass = new FForwardPass(this);
    if (!ForwardPass->Initialize(Resources))
        return false;

    TonemapPass = new FTonemapPass(this);
    if (!TonemapPass->Initialize(Resources))
        return false;

    FXAAPass = new FFXAAPass(this);
    if (!FXAAPass->Initialize(Resources))
        return false;

    return true;
}

void FSceneRenderer::Tick(FScene* Scene)
{
    Resources.BackBuffer = Resources.MainViewport->GetBackBuffer();

    // Clear the images that were debug-able last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--BEGIN FRAME--");
    CommandList.BeginFrame();

    GRHICommandExecutor.Tick();

    if (ResizeEvent)
    {
        // Check if we resized and update the Viewport-size on the RHIThread
        FRHIViewport* Viewport = Resources.MainViewport.Get();

        uint32 NewWidth  = ResizeEvent->GetWidth();
        uint32 NewHeight = ResizeEvent->GetHeight();
        if ((Resources.CurrentWidth != NewWidth || Resources.CurrentHeight != NewHeight) && NewWidth > 0 && NewHeight > 0)
        {
            CommandList.ResizeViewport(Viewport, NewWidth, NewHeight);
            LOG_INFO("Resized between this and the previous frame. From: w=%d h=%d, To: w=%d h=%d", Resources.CurrentWidth, Resources.CurrentHeight, NewWidth, NewHeight);

            // TODO: Resources should not require a CommandList to be released safely
            if (!DepthPrePass->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!BasePass->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!TiledLightPass->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!DepthReducePass->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!ScreenSpaceOcclusionPass->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!ShadowMaskRenderPass->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!TemporalAA->ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }
        }

        Resources.CurrentWidth  = NewWidth;
        Resources.CurrentHeight = NewHeight;
        ResizeEvent.Reset();
    }

    CommandList.BeginExternalCapture();

    // Begin capture GPU FrameTime
    FGPUProfiler::Get().BeginGPUFrame(CommandList);

    // Prepare Lights
    Resources.BuildLightBuffers(CommandList, Scene);

    // Update camera-buffer
    // TODO: All matrices needs to be in Transposed the same
    FCamera* Camera = Scene->Camera;
    CameraBuffer.PrevViewProjection          = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjection              = Camera->GetViewProjectionMatrix();
    CameraBuffer.ViewProjectionInv           = Camera->GetViewProjectionInverseMatrix();
    CameraBuffer.ViewProjectionUnjittered    = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjectionInvUnjittered = CameraBuffer.ViewProjectionInv;

    CameraBuffer.View           = Camera->GetViewMatrix();
    CameraBuffer.ViewInv        = Camera->GetViewInverseMatrix();
    CameraBuffer.Projection     = Camera->GetProjectionMatrix();
    CameraBuffer.ProjectionInv  = Camera->GetProjectionInverseMatrix();

    CameraBuffer.Position       = Camera->GetPosition();
    CameraBuffer.Forward        = Camera->GetForward();
    CameraBuffer.Right          = Camera->GetRight();
    CameraBuffer.NearPlane      = Camera->GetNearPlane();
    CameraBuffer.FarPlane       = Camera->GetFarPlane();
    CameraBuffer.AspectRatio    = Camera->GetAspectRatio();
    CameraBuffer.ViewportWidth  = static_cast<float>(Resources.BackBuffer->GetWidth());
    CameraBuffer.ViewportHeight = static_cast<float>(Resources.BackBuffer->GetHeight());

    if (CVarEnableTemporalAA.GetValue())
    {
        const FVector2 CameraJitter    = HaltonState.NextSample();
        const FVector2 ClipSpaceJitter = CameraJitter / FVector2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

        // Add Jitter to projection matrix
        FMatrix4 JitterOffset = FMatrix4::Translation(FVector3(ClipSpaceJitter.x, ClipSpaceJitter.y, 0.0f));
        CameraBuffer.Projection    = CameraBuffer.Projection * JitterOffset;
        CameraBuffer.ProjectionInv = CameraBuffer.Projection.Invert();
        CameraBuffer.ProjectionInv = CameraBuffer.ProjectionInv.Transpose();

        // Calculate new ViewProjection
        CameraBuffer.ViewProjection    = CameraBuffer.View.Transpose() * CameraBuffer.Projection;
        CameraBuffer.ViewProjectionInv = CameraBuffer.ViewProjection.Invert();
        CameraBuffer.ViewProjection    = CameraBuffer.ViewProjection.Transpose();
        CameraBuffer.ViewProjectionInv = CameraBuffer.ViewProjectionInv.Transpose();

        CameraBuffer.PrevJitter = CameraBuffer.Jitter;
        CameraBuffer.Jitter     = ClipSpaceJitter;
    }
    else
    {
        CameraBuffer.PrevJitter = FVector2(0.0f);
        CameraBuffer.Jitter     = FVector2(0.0f);
    }

    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(Resources.CameraBuffer.Get(), FBufferRegion(0, sizeof(FCameraHLSL)), &CameraBuffer);
    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    for (FMaterial* Material : Scene->Materials)
    {
        // TODO: Only do this once?
        DepthPrePass->InitializePipelineState(Material, Resources);
        BasePass->InitializePipelineState(Material, Resources);
        PointLightRenderPass->InitializePipelineState(Material, Resources);
        CascadedShadowsRenderPass->InitializePipelineState(Material, Resources);

        if (Material->IsBufferDirty())
        {
            Material->BuildBuffer(CommandList);
        }
    }
    
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Albedo].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Normal].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Material].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_ViewNormal].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Velocity].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    // PrePass
    if (CVarPrePassEnabled.GetValue())
    {
        DepthPrePass->Execute(CommandList, Resources, Scene);
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

        CommandList.TransitionTexture(ShadingImage.Get(), EResourceAccess::ShadingRateSource, EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(ShadingRatePipeline.Get());

        FRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        CommandList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

        CommandList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);

        CommandList.TransitionTexture(ShadingImage.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::ShadingRateSource);

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
        BasePass->Execute(CommandList, Resources, Scene);
    }

    // Depth Reduce
    if (CVarPrePassDepthReduce.GetValue())
    {
        DepthReducePass->Execute(CommandList, Resources, Scene);
    }

    // RayTracing PrePass
    if (false /*GRHISupportsRayTracing*/)
    {
        GPU_TRACE_SCOPE(CommandList, "Ray Tracing");
        RayTracer.PreRender(CommandList, Resources, Scene);
    }

    // Start recording the main CommandList
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Albedo].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Albedo]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_Albedo],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Normal].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_Normal],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_ViewNormal].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_ViewNormal]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_ViewNormal],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Velocity].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Velocity]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_Velocity],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Material].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Material]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_Material],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    
    // SSAO
    CommandList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    if (CVarEnableSSAO.GetValue())
    {
        ScreenSpaceOcclusionPass->Execute(CommandList, Resources);
    }
    else
    {
        CommandList.ClearUnorderedAccessView(Resources.SSAOBuffer->GetUnorderedAccessView(), FVector4{ 1.0f, 1.0f, 1.0f, 1.0f });
    }

    CommandList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.SSAOBuffer->GetShaderResourceView()),
        Resources.SSAOBuffer,
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    // Render Shadows
    if (CVarShadowsEnabled.GetValue())
    {
        // Point Lights
        if (CVarPointLightShadowsEnabled.GetValue())
        {
            PointLightRenderPass->Execute(CommandList, Resources, Scene);
        }

        // Directional Light
        if (CVarSunShadowsEnabled.GetValue())
        {
            CascadeGenerationPass->Execute(CommandList, Resources);
            CascadedShadowsRenderPass->Execute(CommandList, Resources, Scene);
        }
    }

    // ShadowMask and GBuffer
    CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess);
    CommandList.TransitionTexture(Resources.BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.Skylight.IrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(Resources.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

    if (CVarShadowMaskEnabled.GetValue())
    {
        ShadowMaskRenderPass->Execute(CommandList, Resources);
    }
    else
    {
        CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        const FVector4 MaskClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        CommandList.ClearUnorderedAccessView(Resources.DirectionalShadowMask->GetUnorderedAccessView(), MaskClearColor);
            
        const FVector4 DebugClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.ClearUnorderedAccessView(Resources.CascadeIndexBuffer->GetUnorderedAccessView(), DebugClearColor);
            
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    // Main LightPass
    TiledLightPass->Execute(CommandList, Resources);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::RenderTarget);

    // Skybox Pass
    if (CVarSkyboxEnabled.GetValue())
    {
        SkyboxRenderPass->Execute(CommandList, Resources, Scene);
    }

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.DirectionalShadowMask->GetShaderResourceView()),
        Resources.DirectionalShadowMask,
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.ShadowMapCascades[0]->GetShaderResourceView()),
        Resources.ShadowMapCascades[0],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.ShadowMapCascades[1]->GetShaderResourceView()),
        Resources.ShadowMapCascades[1],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.ShadowMapCascades[2]->GetShaderResourceView()),
        Resources.ShadowMapCascades[2],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.ShadowMapCascades[3]->GetShaderResourceView()),
        Resources.ShadowMapCascades[3],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(Resources.Skylight.IrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(Resources.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.IntegrationLUT->GetShaderResourceView()),
        Resources.IntegrationLUT,
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    // Forward Pass
    if (false/*!Resources.ForwardVisibleCommands.IsEmpty()*/)
    {
        ForwardPass->Execute(CommandList, Resources, Scene);
    }

    // Debug PointLights
    if (CVarDrawPointLights.GetValue())
    {
        DebugRenderer.RenderPointLights(CommandList, Resources, Scene);
    }

    // Debug AABBs
    if (CVarDrawAABBs.GetValue())
    {
        DebugRenderer.RenderObjectAABBs(CommandList, Resources, Scene);
    }

    // Temporal AA
    if (CVarEnableTemporalAA.GetValue())
    {
        CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::RenderTarget, EResourceAccess::UnorderedAccess);

        TemporalAA->Execute(CommandList, Resources);

        CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
    }
    else
    {
        CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource);
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource);
    }

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_Depth],
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    // FXAA
    if (CVarEnableFXAA.GetValue())
    {
        FXAAPass->Execute(CommandList, Resources, Scene);
    }

    // Perform ToneMapping and Blit to BackBuffer
    TonemapPass->Execute(CommandList, Resources, Scene);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.FinalTarget->GetShaderResourceView()),
        Resources.FinalTarget,
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin UI Render");

    {
    #if SUPPORT_VARIABLE_RATE_SHADING
        if (RHISupportsVariableRateShading())
        {
            CommandList.SetShadingRate(EShadingRate::VRS_1x1);
            CommandList.SetShadingRateImage(nullptr);
        }
    #endif

        TRACE_SCOPE("Render UI");
        FApplication::Get().DrawWindows(CommandList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End UI Render");

    CommandList.TransitionTexture(Resources.BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);

    FGPUProfiler::Get().EndGPUFrame(CommandList);

    CommandList.EndExternalCapture();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--END FRAME--");

    CommandList.PresentViewport(Resources.MainViewport.Get(), CVarVSyncEnabled.GetValue());
    CommandList.EndFrame();

    CommandList.FlushGarbageCollection();

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

        GRHICommandExecutor.ExecuteCommandList(CommandList);
        FrameStatistics = GRHICommandExecutor.GetStatistics();
    }
}

void FSceneRenderer::ResizeResources(const FWindowEvent& Event)
{
    ResizeEvent.Emplace(Event);
}

bool FSceneRenderer::InitShadingImage()
{
    if (GRHIShadingRateTier != EShadingRateTier::Tier2 || GRHIShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.MainViewport->GetWidth() / GRHIShadingRateImageTileSize;
    const uint32 Height = Resources.MainViewport->GetHeight() / GRHIShadingRateImageTileSize;

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    ShadingImage = RHICreateTexture(TextureInfo, EResourceAccess::ShadingRateSource);
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

    ShadingRateShader = RHICreateComputeShader(ShaderCode);
    if (!ShadingRateShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer PSOInitializer(ShadingRateShader.Get());
    ShadingRatePipeline = RHICreateComputePipelineState(PSOInitializer);
    if (!ShadingRatePipeline)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}
