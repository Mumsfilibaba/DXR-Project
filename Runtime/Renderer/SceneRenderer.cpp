#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Time/Timespan.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Engine.h"
#include "Engine/Resources/Model.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Renderer/SceneRenderer.h"
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
    FConsoleCommandDelegate::CreateLambda([]()
    {
        GFreezeRendering = !GFreezeRendering;
    }));

FSceneRenderer::FSceneRenderer()
    : TextureDebugger(nullptr)
    , InfoWindow(nullptr)
    , GPUProfilerWindow(nullptr)
    , SettingsWindow(nullptr)
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

    if (IImguiPlugin::IsEnabled())
    {
        TextureDebugger.Reset();
        InfoWindow.Reset();
        GPUProfilerWindow.Reset();
        SettingsWindow.Reset();
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

    if (false/*RHIDeviceInfo::SupportsRayTracing*/)
    {
        if (!RayTracer.Initialize(Resources))
        {
            return false;
        }
    }

    // Register ImGui Windows
    if (IImguiPlugin::IsEnabled())
    {
        TextureDebugger   = MakeSharedPtr<FTextureDebugWidget>();
        InfoWindow        = MakeSharedPtr<FRendererInfoWidget>(this);
        GPUProfilerWindow = MakeSharedPtr<FGPUProfilerWidget>();
        SettingsWindow    = MakeSharedPtr<FRendererSettingsWidget>();
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

void FSceneRenderer::BeginFrame()
{
	FRHICommandListExecutor::Get().Tick();

	// Clear the images that were "debug-able" last frame 
	TextureDebugger->ClearImages();

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

		const bool bEnableVSync = CVarVSyncEnabled.GetValue();
		for (FRHISwapChainRef SwapChain : SwapChainsToPrepare)
		{
			FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
			CommandList.TransitionTexture(BackBuffer, FRHITextureTransition::Make(EResourceAccess::Present, EResourceAccess::RenderTarget));
		}

        SwapChainsToPrepare.Clear();
	}
}

void FSceneRenderer::RenderSceneView(const FSceneRenderView& SceneRenderView)
{
	if (RenderSettings::NeedsResize())
	{
        ResizeResources(RenderSettings::GetRenderWidth(), RenderSettings::GetRenderHeight());
	}

    // Cast and cache the current scene being rendered
    FScene* CurrentScene = static_cast<FScene*>(SceneRenderView.Scene);
	
    // Prepare Lights
	Resources.BuildLightBuffers(CommandList, CurrentScene);

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

	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Albedo].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Normal].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Material].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Velocity].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite));

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

		CommandList.TransitionTexture(ShadingImage.Get(), FRHITextureTransition::Make(EResourceAccess::ShadingRateSource, EResourceAccess::UnorderedAccess));

		CommandList.SetComputePipelineState(ShadingRatePipeline.Get());

		FRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
		CommandList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

		CommandList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);

		CommandList.TransitionTexture(ShadingImage.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::ShadingRateSource));

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

	// Depth Reduce
	if (CVarCSMTightFrustum.GetValue())
	{
		DepthReducePass->Execute(CommandList, Resources, CurrentScene);
	}

	// RayTracing PrePass
	if (false /*RHIDeviceInfo::SupportsRayTracing*/)
	{
		GPU_TRACE_SCOPE(CommandList, "Ray Tracing");
		RayTracer.PreRender(CommandList, Resources, CurrentScene);
	}

	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Albedo].Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Normal].Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Velocity].Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Material].Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource));
	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource));
	CommandList.TransitionTexture(Resources.SSAOBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Albedo]->GetShaderResourceView()),
		Resources.GBuffer[GBufferIndex_Albedo], EResourceAccess::NonPixelShaderResource);

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView()),
		Resources.GBuffer[GBufferIndex_Normal], EResourceAccess::NonPixelShaderResource);

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Velocity]->GetShaderResourceView()),
		Resources.GBuffer[GBufferIndex_Velocity], EResourceAccess::NonPixelShaderResource);

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Material]->GetShaderResourceView()),
		Resources.GBuffer[GBufferIndex_Material], EResourceAccess::NonPixelShaderResource);

	// SSAO
	if (CVarEnableSSAO.GetValue())
	{
		ScreenSpaceOcclusionPass->Execute(CommandList, Resources);
	}
	else
	{
		CommandList.ClearUnorderedAccessView(Resources.SSAOBuffer->GetUnorderedAccessView(), FVector4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	CommandList.TransitionTexture(Resources.SSAOBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.SSAOBuffer->GetShaderResourceView()),
		Resources.SSAOBuffer, EResourceAccess::NonPixelShaderResource);

	// Render Shadows
	const bool bEnableShadows    = CVarShadowsEnabled.GetValue();
	const bool bEnableSunShadows = CVarSunShadowsEnabled.GetValue();

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
	CommandList.TransitionTexture(Resources.FinalTarget.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess));
	CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

	if (CurrentScene)
	{
		if (FSceneSkyLight* SkyLight = CurrentScene->SkyLight)
		{
			CommandList.TransitionTexture(SkyLight->DiffuseCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
			CommandList.TransitionTexture(SkyLight->SpecularCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
		}
	}

	// In order to render the shadow-mask, we want all these features to be enabled
	const bool bEnableShadowMask = CVarShadowMaskEnabled.GetValue();
	if (bEnableShadows && bEnableShadowMask && bEnableSunShadows)
	{
		ShadowMaskRenderPass->Execute(CommandList, Resources);
	}
	else
	{
		CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));
		CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));

		const FVector4 MaskClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		CommandList.ClearUnorderedAccessView(Resources.DirectionalShadowMask->GetUnorderedAccessView(), MaskClearColor);

		const FVector4 DebugClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		CommandList.ClearUnorderedAccessView(Resources.CascadeIndexBuffer->GetUnorderedAccessView(), DebugClearColor);

		CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
		CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
	}

	// Main LightPass
	TiledLightPass->Execute(CommandList, Resources, CurrentScene);

	CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite));
	CommandList.TransitionTexture(Resources.FinalTarget.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::RenderTarget));

	// Skybox Pass
	if (CVarSkyboxEnabled.GetValue())
	{
		SkyboxRenderPass->Execute(CommandList, Resources, CurrentScene);
	}

	CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.DirectionalShadowMask->GetShaderResourceView()),
		Resources.DirectionalShadowMask, EResourceAccess::NonPixelShaderResource);

	for (int32 Index = 0; Index < NUM_SHADOW_CASCADES; Index++)
	{
		AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.ShadowCascadesSRVs[Index].Get()),
			Resources.ShadowCascades, EResourceAccess::NonPixelShaderResource);
	}

	if (CurrentScene)
	{
		if (FSceneSkyLight* SkyLight = CurrentScene->SkyLight)
		{
			CommandList.TransitionTexture(SkyLight->DiffuseCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
			CommandList.TransitionTexture(SkyLight->SpecularCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
		}
	}

	CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.IntegrationLUT->GetShaderResourceView()),
		Resources.IntegrationLUT, EResourceAccess::PixelShaderResource);

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
		CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource));
		CommandList.TransitionTexture(Resources.FinalTarget.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::UnorderedAccess));

		TemporalAA->Execute(CommandList, Resources);

		CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
		CommandList.TransitionTexture(Resources.FinalTarget.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource));
	}
	else
	{
		CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource));
		CommandList.TransitionTexture(Resources.FinalTarget.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));
	}

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView()),
		Resources.GBuffer[GBufferIndex_Depth], EResourceAccess::PixelShaderResource);

	// FXAA
	if (CVarEnableFXAA.GetValue())
	{
		FXAAPass->Execute(CommandList, SceneRenderView, Resources);
	}

	// Perform ToneMapping and blit to BackBuffer
	TonemapPass->Execute(CommandList, SceneRenderView, Resources);

	AddDebugTexture(MakeSharedRef<FRHIShaderResourceView>(Resources.FinalTarget->GetShaderResourceView()),
		Resources.FinalTarget, EResourceAccess::PixelShaderResource);
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
            CommandList.TransitionTexture(BackBuffer, FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::Present));
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

        LOG_INFO("Changed render-resolution. From: w=%d h=%d, To: w=%d h=%d", 
            Resources.CurrentRenderWidth, Resources.CurrentRenderHeight, InWidth, InHeight);
		
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
    if (RHIDeviceInfo::ShadingRateTier != EShadingRateTier::Tier2 || RHIDeviceInfo::ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.CurrentRenderWidth / RHIDeviceInfo::ShadingRateImageTileSize;
    const uint32 Height = Resources.CurrentRenderHeight / RHIDeviceInfo::ShadingRateImageTileSize;

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture);
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

    FRHIComputePipelineStateInitializer PSOInitializer(ShadingRateShader.Get());
    ShadingRatePipeline = FRHI::Get()->CreateComputePipelineState(PSOInitializer);

    if (!ShadingRatePipeline)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}
