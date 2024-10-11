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

static TAutoConsoleVariable<bool> CVarDrawOcclusionVolumes(
    "Renderer.Debug.DrawOcclusionVolumes",
    "Draws all the objects bounding boxes that are used for occlusion culling",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarDrawPointLights(
    "Renderer.Debug.DrawPointLights", 
    "Draws all the PointLights as spheres with the light-color",
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

static TAutoConsoleVariable<bool> CVarPrePassDepthReduce(
    "Renderer.PrePass.DepthReduce",
    "Set to true to reduce the DepthBuffer to find the Min- and Max Depth in the DepthBuffer",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarBasePassOcclusionCulling(
    "Renderer.BasePass.OcclusionCulling",
    "Should occlusion culling be performed or not",
    false,
    EConsoleVariableFlags::Default);

static FAutoConsoleCommand CVarFreezeRendering(
    "Renderer.FreezeRendering",
    "Freezes the updating of Frustum and Occlusion culling",
    FConsoleCommandDelegate::CreateLambda([]()
    {
        GFreezeRendering = !GFreezeRendering;
    }));

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
    , OcclusionPass(nullptr)
    , DepthReducePass(nullptr)
    , TiledLightPass(nullptr)
    , PointLightRenderPass(nullptr)
    , CascadeGenerationPass(nullptr)
    , CascadedShadowsRenderPass(nullptr)
    , ShadowMaskRenderPass(nullptr)
    , ScreenSpaceOcclusionPass(nullptr)
    , SkyboxRenderPass(nullptr)
    , TemporalAA(nullptr)
    , ForwardPass(nullptr)
    , FXAAPass(nullptr)
    , TonemapPass(nullptr)
    , LightProbeRenderer(nullptr)
    , DebugRenderer(nullptr)
    , RayTracer(this)
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
    SAFE_DELETE(OcclusionPass);
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
    SAFE_DELETE(LightProbeRenderer);
    SAFE_DELETE(DebugRenderer);

    RayTracer.Release();

    Resources.Release();

    ShadingImage.Reset();
    ShadingRatePipeline.Reset();
    ShadingRateShader.Reset();

    TimestampQueries.Reset();

    FrameStatistics.Reset();

    if (IImguiPlugin::IsEnabled())
    {
        TextureDebugger.Reset();
        InfoWindow.Reset();
        GPUProfilerWindow.Reset();
    }
}

bool FSceneRenderer::Initialize()
{
    TSharedPtr<FSceneViewport> SceneViewport = GEngine->GetSceneViewport();
    if (!SceneViewport)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.MainViewport     = SceneViewport->GetViewportRHI();
        Resources.BackBufferFormat = Resources.MainViewport->GetColorFormat();
        Resources.CurrentWidth     = Resources.DesiredWidth  = Resources.MainViewport->GetWidth();
        Resources.CurrentHeight    = Resources.DesiredHeight = Resources.MainViewport->GetHeight();
    }

    if (!FApplicationInterface::IsInitialized())
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
    FRHIVertexLayoutInitializerList InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0,  0, EVertexInputClass::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, sizeof(FVertexNormal),   1, 0,  1, EVertexInputClass::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, sizeof(FVertexNormal),   1, 12, 2, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 2, 0,  3, EVertexInputClass::Vertex, 0 },
    };

    Resources.MeshInputLayout = RHICreateVertexLayout(InputLayout);
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
        SamplerStateInfo.Filter         = ESamplerFilter::Comparison_MinMagMipPoint;
        SamplerStateInfo.ComparisonFunc = EComparisonFunc::LessEqual;
        SamplerStateInfo.BorderColor    = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
        SamplerStateInfo.MinLOD         = 0.0f;

        Resources.ShadowSamplerPointCmp = RHICreateSamplerState(SamplerStateInfo);
        if (!Resources.ShadowSamplerPointCmp)
        {
            DEBUG_BREAK();
            return false;
        }

        SamplerStateInfo.Filter = ESamplerFilter::Comparison_MinMagMipLinear;

        Resources.ShadowSamplerLinearCmp = RHICreateSamplerState(SamplerStateInfo);
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

        Resources.PointLightShadowSampler = RHICreateSamplerState(SamplerStateInfo);
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

    if (false/*GRHISupportsRayTracing*/)
    {
        if (!RayTracer.Initialize(Resources))
        {
            return false;
        }
    }

    // Copy over the texture
    {
        LightProbeRenderer->RenderSkyLightProbe(CommandList, Resources);
        
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
    if (IImguiPlugin::IsEnabled())
    {
        TextureDebugger   = MakeSharedPtr<FRenderTargetDebugWindow>();
        InfoWindow        = MakeSharedPtr<FRendererInfoWindow>(this);
        GPUProfilerWindow = MakeSharedPtr<FGPUProfilerWindow>();
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

    OcclusionPass = new FOcclusionPass(this);
    if (!OcclusionPass->Initialize(Resources))
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

void FSceneRenderer::Tick(FScene* Scene)
{
    TSharedPtr<FWindow> EngineWindow = GEngine->GetEngineWindow();
    Resources.BackBuffer = Resources.MainViewport->GetBackBuffer();

    // Clear the images that were debug-able last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--BEGIN FRAME--");
    CommandList.BeginFrame();

    GRHICommandExecutor.Tick();
    
    const FIntVector2 CurrentSize = EngineWindow->GetScreenSize();
    Resources.DesiredWidth  = CurrentSize.x;
    Resources.DesiredHeight = CurrentSize.y;

    if (Resources.DesiredWidth != Resources.CurrentWidth || Resources.DesiredHeight != Resources.CurrentHeight)
    {
        // Check if we resized and update the Viewport-size on the RHIThread
        FRHIViewport* Viewport = Resources.MainViewport.Get();

        uint32 NewWidth  = Resources.DesiredWidth;
        uint32 NewHeight = Resources.DesiredHeight;
        if ((Resources.CurrentWidth != NewWidth || Resources.CurrentHeight != NewHeight) && NewWidth > 0 && NewHeight > 0)
        {
            CommandList.ResizeViewport(Viewport, NewWidth, NewHeight);
            LOG_INFO("Resized between this and the previous frame. From: w=%d h=%d, To: w=%d h=%d", Resources.CurrentWidth, Resources.CurrentHeight, NewWidth, NewHeight);

            // TODO: Resources should not require a CommandList to be released safely
            if (!DepthPrePass->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!BasePass->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!TiledLightPass->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!DepthReducePass->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!ScreenSpaceOcclusionPass->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!ShadowMaskRenderPass->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }

            if (!TemporalAA->CreateResources(Resources, NewWidth, NewHeight))
            {
                DEBUG_BREAK();
                return;
            }
        }

        Resources.CurrentWidth  = NewWidth;
        Resources.CurrentHeight = NewHeight;
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
    CameraBuffer.View                        = Camera->GetViewMatrix();
    CameraBuffer.ViewInv                     = Camera->GetViewInverseMatrix();
    CameraBuffer.Projection                  = Camera->GetProjectionMatrix();
    CameraBuffer.ProjectionInv               = Camera->GetProjectionInverseMatrix();
    CameraBuffer.Position                    = Camera->GetPosition();
    CameraBuffer.Forward                     = Camera->GetForward();
    CameraBuffer.Right                       = Camera->GetRight();
    CameraBuffer.NearPlane                   = Camera->GetNearPlane();
    CameraBuffer.FarPlane                    = Camera->GetFarPlane();
    CameraBuffer.AspectRatio                 = Camera->GetAspectRatio();
    CameraBuffer.ViewportWidth               = static_cast<float>(Resources.BackBuffer->GetWidth());
    CameraBuffer.ViewportHeight              = static_cast<float>(Resources.BackBuffer->GetHeight());

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

    // Update occlusion
    if (!GFreezeRendering)
    {
        if (CVarBasePassOcclusionCulling.GetValue())
        {
            for (FProxySceneComponent* Component : Scene->VisiblePrimitives)
            {
                Component->UpdateOcclusion();
            }
        }
        else
        {
            for (FProxySceneComponent* Component : Scene->VisiblePrimitives)
            {
                Component->NumFramesOccluded = 0;
            }
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

    // Occlusion Pass
    if (CVarBasePassOcclusionCulling.GetValue())
    {
        OcclusionPass->Execute(CommandList, Resources, Scene);
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
            if (!GFreezeRendering)
            {
                CascadeGenerationPass->Execute(CommandList, Resources);
            }

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
        MakeSharedRef<FRHIShaderResourceView>(Resources.ShadowMapCascades->GetShaderResourceView()),
        Resources.ShadowMapCascades,
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
    // if (!Resources.ForwardVisibleCommands.IsEmpty())
    // {
        // ForwardPass->Execute(CommandList, Resources, Scene);
    // }

    // Debug PointLights
    if (CVarDrawPointLights.GetValue())
    {
        DebugRenderer->RenderPointLights(CommandList, Resources, Scene);
    }

    // Debug AABBs
    if (CVarDrawAABBs.GetValue())
    {
        DebugRenderer->RenderObjectAABBs(CommandList, Resources, Scene);
    }

    // Debug Occlusion Boxes
    if (CVarDrawOcclusionVolumes.GetValue())
    {
        DebugRenderer->RenderOcclusionVolumes(CommandList, Resources, Scene);
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

        if (IImguiPlugin::IsEnabled())
        {
            IImguiPlugin::Get().Draw(CommandList);
        }
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

void FSceneRenderer::ResizeResources(uint32 InWidth, uint32 InHeight)
{
    Resources.DesiredWidth  = InWidth;
    Resources.DesiredHeight = InHeight;
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
