#include "SceneRenderer.h"
#include "Debug/GPUProfiler.h"
#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Application/Application.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Engine.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
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

static TAutoConsoleVariable<bool> CVarFXAADebug(
    "Renderer.Debug.FXAADebug",
    "Enables FXAA (Anti-Aliasing) Debugging mode",
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

    Renderer->OnWindowResize(WindowEvent);
    return FResponse::Handled();
}

FSceneRenderer::FSceneRenderer()
    : TextureDebugger(nullptr)
    , InfoWindow(nullptr)
    , GPUProfilerWindow(nullptr)
    , CommandList()
    , Resources()
    , LightSetup()
    , CameraBuffer()
    , HaltonState()
    , DeferredRenderer(this)
    , ShadowMapRenderer(this)
    , SSAORenderer(this)
    , LightProbeRenderer(this)
    , SkyboxRenderPass(this)
    , ForwardRenderer(this)
    , RayTracer(this)
    , DebugRenderer(this)
    , TemporalAA(this)
    , ShadingImage(nullptr)
    , ShadingRatePipeline(nullptr)
    , ShadingRateShader(nullptr)
    , PostPSO(nullptr)
    , PostShader(nullptr)
    , FXAAPSO(nullptr)
    , FXAAShader(nullptr)
    , FXAADebugPSO(nullptr)
    , FXAADebugShader(nullptr)
    , TimestampQueries(nullptr)
    , FrameStatistics()
{
}

FSceneRenderer::~FSceneRenderer()
{
    GRHICommandExecutor.WaitForGPU();

    CommandList.Reset();

    DeferredRenderer.Release();
    ShadowMapRenderer.Release();
    SSAORenderer.Release();
    LightProbeRenderer.Release();
    SkyboxRenderPass.Release();
    ForwardRenderer.Release();
    RayTracer.Release();
    DebugRenderer.Release();
    TemporalAA.Release();

    Resources.Release();
    LightSetup.Release();

    PostPSO.Reset();
    PostShader.Reset();
    FXAAPSO.Reset();
    FXAAShader.Reset();
    FXAADebugPSO.Reset();
    FXAADebugShader.Reset();

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

    FRHIBufferDesc CBDesc(sizeof(FCameraBuffer), sizeof(FCameraBuffer), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
    Resources.CameraBuffer = RHICreateBuffer(CBDesc, EResourceAccess::Common, nullptr);
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
        FRHISamplerStateDesc Initializer;
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
        FRHISamplerStateDesc Initializer;
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

    if (!InitAA())
    {
        return false;
    }

    if (!InitShadingImage())
    {
        return false;
    }

    if (!DebugRenderer.Initialize(Resources))
    {
        return false;
    }

    if (!LightSetup.Initialize())
    {
        return false;
    }

    if (!DeferredRenderer.Initialize(Resources))
    {
        return false;
    }

    if (!ShadowMapRenderer.Initialize(LightSetup, Resources))
    {
        return false;
    }

    if (!SSAORenderer.Initialize(Resources))
    {
        return false;
    }

    if (!LightProbeRenderer.Initialize(LightSetup, Resources))
    {
        return false;
    }

    if (!SkyboxRenderPass.Initialize(Resources))
    {
        return false;
    }

    if (!ForwardRenderer.Initialize(Resources))
    {
        return false;
    }

    if (!TemporalAA.Initialize(Resources))
    {
        return false;
    }

    if (false/*FHardwareSupport::bRayTracing*/)
    {
        if (!RayTracer.Initialize(Resources))
        {
            return false;
        }
    }

    // Copy over the texture
    {
        LightProbeRenderer.RenderSkyLightProbe(CommandList, LightSetup, Resources);
        
        FRHITextureDesc IrradianceProbeDesc = LightSetup.Skylight.IrradianceMap->GetDesc();
        IrradianceProbeDesc.UsageFlags = ETextureUsageFlags::ShaderResource;

        FRHITextureDesc SpecularIrradianceProbeDesc = LightSetup.Skylight.SpecularIrradianceMap->GetDesc();
        SpecularIrradianceProbeDesc.UsageFlags = ETextureUsageFlags::ShaderResource;

        FRHITextureRef IrradianceMap = RHICreateTexture(IrradianceProbeDesc, EResourceAccess::CopyDest);
        if (!IrradianceMap)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            IrradianceMap->SetDebugName("Irradiance Map");
        }

        FRHITextureRef SpecularIrradianceMap = RHICreateTexture(SpecularIrradianceProbeDesc, EResourceAccess::CopyDest);
        if (!SpecularIrradianceMap)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            SpecularIrradianceMap->SetDebugName("Specular Irradiance Map");
        }

        CommandList.TransitionTexture(LightSetup.Skylight.IrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopySource);
        CommandList.TransitionTexture(LightSetup.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopySource);

        CommandList.CopyTexture(IrradianceMap.Get(), LightSetup.Skylight.IrradianceMap.Get());
        CommandList.CopyTexture(SpecularIrradianceMap.Get(), LightSetup.Skylight.SpecularIrradianceMap.Get());

        CommandList.TransitionTexture(IrradianceMap.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);
        CommandList.TransitionTexture(SpecularIrradianceMap.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        CommandList.DestroyResource(LightSetup.Skylight.IrradianceMap.Get());
        CommandList.DestroyResource(LightSetup.Skylight.SpecularIrradianceMap.Get());

        LightSetup.Skylight.IrradianceMap         = IrradianceMap;
        LightSetup.Skylight.SpecularIrradianceMap = SpecularIrradianceMap;

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

void FSceneRenderer::PerformFXAA(FRHICommandList& InCommandList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCommandList, "Begin FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(InCommandList, "FXAA");

    struct FFXAASettings
    {
        float Width;
        float Height;
    } Settings;

    const float RenderWidth  = static_cast<float>(Resources.BackBuffer->GetWidth());
    const float RenderHeight = static_cast<float>(Resources.BackBuffer->GetHeight());

    Settings.Width  = RenderWidth;
    Settings.Height = RenderHeight;

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    InCommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    InCommandList.SetScissorRect(ScissorRegion);

    FRHIRenderPassDesc RenderPass;
    RenderPass.NumRenderTargets = 1;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    InCommandList.BeginRenderPass(RenderPass);

    FRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    if (CVarFXAADebug.GetValue())
    {
        InCommandList.SetGraphicsPipelineState(FXAADebugPSO.Get());
        InCommandList.SetShaderResourceView(FXAADebugShader.Get(), FinalTargetSRV, 0);
        InCommandList.SetSamplerState(FXAADebugShader.Get(), Resources.FXAASampler.Get(), 0);
        InCommandList.Set32BitShaderConstants(FXAADebugShader.Get(), &Settings, 2);
    }
    else
    {
        InCommandList.SetGraphicsPipelineState(FXAAPSO.Get());
        InCommandList.SetShaderResourceView(FXAAShader.Get(), FinalTargetSRV, 0);
        InCommandList.SetSamplerState(FXAAShader.Get(), Resources.FXAASampler.Get(), 0);
        InCommandList.Set32BitShaderConstants(FXAAShader.Get(), &Settings, 2);
    }

    InCommandList.DrawInstanced(3, 1, 0, 0);

    InCommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(InCommandList, "End FXAA");
}

void FSceneRenderer::PerformBackBufferBlit(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin Draw BackBuffer");

    TRACE_SCOPE("Draw to BackBuffer");

    const float RenderWidth  = static_cast<float>(Resources.BackBuffer->GetWidth());
    const float RenderHeight = static_cast<float>(Resources.BackBuffer->GetHeight());

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    InCmdList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    InCmdList.SetScissorRect(ScissorRegion);

    FRHIRenderPassDesc RenderPass;
    RenderPass.NumRenderTargets = 1;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Load);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    InCmdList.BeginRenderPass(RenderPass);

    InCmdList.SetGraphicsPipelineState(PostPSO.Get());
    
    FRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    InCmdList.SetShaderResourceView(PostShader.Get(), FinalTargetSRV, 0);
    InCmdList.SetSamplerState(PostShader.Get(), Resources.GBufferSampler.Get(), 0);

    InCmdList.DrawInstanced(3, 1, 0, 0);

    InCmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End Draw BackBuffer");
}

void FSceneRenderer::Tick(FRendererScene* Scene)
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
            if (!DeferredRenderer.ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!SSAORenderer.ResizeResources(CommandList, Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!ShadowMapRenderer.ResizeResources(CommandList, NewWidth, NewHeight, LightSetup))
            {
                DEBUG_BREAK();
                return;
            }

            if (!TemporalAA.ResizeResources(CommandList, Resources, NewWidth, NewHeight))
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
    LightSetup.BeginFrame(CommandList, Scene);

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
    CommandList.UpdateBuffer(Resources.CameraBuffer.Get(), FBufferRegion(0, sizeof(FCameraBuffer)), &CameraBuffer);
    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    for (FMaterial* Material : Scene->Materials)
    {
        // TODO: Only do this once?
        DeferredRenderer.InitializePipelineState(Material, Resources);

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
        DeferredRenderer.RenderPrePass(CommandList, Resources, Scene);
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
        DeferredRenderer.RenderBasePass(CommandList, Resources, Scene);
    }

    // RayTracing PrePass
    if (false /*FHardwareSupport::bRayTracing*/)
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
        GPU_TRACE_SCOPE(CommandList, "SSAO");
        SSAORenderer.Render(CommandList, Resources);
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
            ShadowMapRenderer.RenderPointLightShadows(CommandList, LightSetup, Scene);
        }

        // Directional Light
        if (CVarSunShadowsEnabled.GetValue())
        {
            ShadowMapRenderer.RenderDirectionalLightShadows(CommandList, LightSetup, Resources, Scene);
        }
    }

    // ShadowMask and GBuffer
    {
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionTexture(Resources.BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);
        CommandList.TransitionTexture(LightSetup.Skylight.IrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(LightSetup.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

        if (CVarShadowMaskEnabled.GetValue())
        {
            ShadowMapRenderer.RenderShadowMasks(CommandList, LightSetup, Resources);
        }
        else
        {
            CommandList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
            CommandList.TransitionTexture(LightSetup.CascadeIndexBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

            const FVector4 MaskClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            CommandList.ClearUnorderedAccessView(LightSetup.DirectionalShadowMask->GetUnorderedAccessView(), MaskClearColor);
            
            const FVector4 DebugClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.ClearUnorderedAccessView(LightSetup.CascadeIndexBuffer->GetUnorderedAccessView(), DebugClearColor);
            
            CommandList.TransitionTexture(LightSetup.CascadeIndexBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
            CommandList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        }

        DeferredRenderer.RenderDeferredTiledLightPass(CommandList, Resources, LightSetup);
    }

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::RenderTarget);

    // Skybox Pass
    if (CVarSkyboxEnabled.GetValue())
    {
        SkyboxRenderPass.Render(CommandList, Resources, Scene);
    }

    CommandList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(LightSetup.DirectionalShadowMask->GetShaderResourceView()),
        LightSetup.DirectionalShadowMask,
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[0]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[0],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[1]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[1],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[2]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[2],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[3]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[3],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(LightSetup.Skylight.IrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(LightSetup.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.IntegrationLUT->GetShaderResourceView()),
        Resources.IntegrationLUT,
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    // Forward Pass
    if (false/*!Resources.ForwardVisibleCommands.IsEmpty()*/)
    {
        GPU_TRACE_SCOPE(CommandList, "Forward Pass");
        ForwardRenderer.Render(CommandList, Resources, LightSetup, Scene);
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

        TemporalAA.Render(CommandList, Resources);

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
        PerformFXAA(CommandList);
    }
    else
    {
        // Render to the BackBuffer
        PerformBackBufferBlit(CommandList);
    }

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.FinalTarget->GetShaderResourceView()),
        Resources.FinalTarget,
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin UI Render");

    {
        TRACE_SCOPE("Render UI");

    #if SUPPORT_VARIABLE_RATE_SHADING
        if (RHISupportsVariableRateShading())
        {
            CommandList.SetShadingRate(EShadingRate::VRS_1x1);
            CommandList.SetShadingRateImage(nullptr);
        }
    #endif

        FApplication::Get().DrawWindows(CommandList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End UI Render");

    CommandList.TransitionTexture(Resources.BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);

    FGPUProfiler::Get().EndGPUFrame(CommandList);

    CommandList.EndExternalCapture();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--END FRAME--");

    CommandList.PresentViewport(Resources.MainViewport.Get(), CVarVSyncEnabled.GetValue());
    CommandList.EndFrame();

    {
        TRACE_SCOPE("ExecuteCommandList");

        GRHICommandExecutor.WaitForOutstandingTasks();
        GRHICommandExecutor.ExecuteCommandList(CommandList);
        FrameStatistics = GRHICommandExecutor.GetStatistics();
    }
}

void FSceneRenderer::OnWindowResize(const FWindowEvent& Event)
{
    ResizeEvent.Emplace(Event);
}

bool FSceneRenderer::InitAA()
{
    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    FRHIVertexShaderRef VShader = RHICreateVertexShader(ShaderCode);
    if (!VShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/PostProcessPS.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    PostShader = RHICreatePixelShader(ShaderCode);
    if (!PostShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc         = EComparisonFunc::Always;
    DepthStencilInitializer.bDepthEnable      = false;
    DepthStencilInitializer.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.VertexInputLayout                      = nullptr;
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = VShader.Get();
    PSOInitializer.ShaderState.PixelShader                = PostShader.Get();
    PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = Resources.BackBufferFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    PostPSO = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PostPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    // FXAA
    FRHISamplerStateDesc SamplerInitializer;
    SamplerInitializer.AddressU = ESamplerMode::Clamp;
    SamplerInitializer.AddressV = ESamplerMode::Clamp;
    SamplerInitializer.AddressW = ESamplerMode::Clamp;
    SamplerInitializer.Filter   = ESamplerFilter::MinMagMipLinear;

    Resources.FXAASampler = RHICreateSamplerState(SamplerInitializer);
    if (!Resources.FXAASampler)
    {
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    FXAAShader = RHICreatePixelShader(ShaderCode);
    if (!FXAAShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSOInitializer.ShaderState.PixelShader = FXAAShader.Get();

    FXAAPSO = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!FXAAPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FXAAPSO->SetDebugName("FXAA PipelineState");
    }

    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_DEBUG", "(1)" }
    };

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel, Defines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    FXAADebugShader = RHICreatePixelShader(ShaderCode);
    if (!FXAADebugShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSOInitializer.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!FXAADebugPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

bool FSceneRenderer::InitShadingImage()
{
    if (FHardwareSupport::ShadingRateTier != EShadingRateTier::Tier2 || FHardwareSupport::ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.MainViewport->GetWidth() / FHardwareSupport::ShadingRateImageTileSize;
    const uint32 Height = Resources.MainViewport->GetHeight() / FHardwareSupport::ShadingRateImageTileSize;

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    ShadingImage = RHICreateTexture(TextureDesc, EResourceAccess::ShadingRateSource);
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
    
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadingImage.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
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
