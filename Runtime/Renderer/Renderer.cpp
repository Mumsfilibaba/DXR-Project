#include "Renderer.h"
#include "Debug/GPUProfiler.h"
#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Application/Application.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Engine.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "RendererCore/TextureFactory.h"

TAutoConsoleVariable<bool> GEnableSSAO(
    "Renderer.Feature.SSAO",
    "Enables Screen-Space Ambient Occlusion",
    true);

TAutoConsoleVariable<bool> GEnableFXAA(
    "Renderer.Feature.FXAA",
    "Enables FXAA for Anti-Aliasing",
    false);
TAutoConsoleVariable<bool> GFXAADebug(
    "Renderer.Debug.FXAADebug",
    "Enables FXAA (Anti-Aliasing) Debugging mode",
    false);

TAutoConsoleVariable<bool> GEnableTemporalAA(
    "Renderer.Feature.TemporalAA",
    "Enables Temporal Anti-Aliasing",
    true);

TAutoConsoleVariable<bool> GEnableVariableRateShading(
    "Renderer.Feature.VariableRateShading",
    "Enables VRS (Variable Rate Shading)",
    false);

// TODO: Remove this, should always be enabled
TAutoConsoleVariable<bool> GPrePassEnabled(
    "Renderer.Feature.PrePass",
    "Enables Pre-Pass",
    true);

TAutoConsoleVariable<bool> GDrawAABBs(
    "Renderer.Debug.DrawAABBs",
    "Draws all the objects bounding boxes (AABB)",
    false);
TAutoConsoleVariable<bool> GDrawPointLights(
    "Renderer.Debug.DrawPointLights", 
    "Draws all the PointLights as spheres with the light-color",
    false);

TAutoConsoleVariable<bool> GVSyncEnabled(
    "Renderer.Feature.VerticalSync",
    "Enables Vertical-Sync", 
    false);
TAutoConsoleVariable<bool> GFrustumCullEnabled(
    "Renderer.Feature.FrustumCulling",
    "Enables Frustum Culling (CPU) for the main scene and for all shadow frustums",
    true);

TAutoConsoleVariable<bool> GRayTracingEnabled(
    "Renderer.Feature.RayTracing",
    "Enables Ray Tracing (Currently broken)",
    false);


FRenderer* FRenderer::GInstance = nullptr;

FRenderer::FRenderer()
    : TextureDebugger(nullptr)
    , InfoWindow(nullptr)
    , GPUProfilerWindow(nullptr)
    , CommandList()
    , Resources()
    , LightSetup()
    , CameraBuffer()
    , HaltonState()
    , DeferredRenderer()
    , ShadowMapRenderer()
    , SSAORenderer()
    , LightProbeRenderer()
    , SkyboxRenderPass()
    , ForwardRenderer()
    , RayTracer()
    , DebugRenderer()
    , TemporalAA()
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

FRenderer::~FRenderer()
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

    if (GEngine && GEngine->MainWindow)
    {
        // GEngine->MainWindow->RemoveOverlayWidget(TextureDebugger);
        TextureDebugger.Reset();

        // GEngine->MainWindow->RemoveOverlayWidget(InfoWindow);
        InfoWindow.Reset();

        // GEngine->MainWindow->RemoveOverlayWidget(InfoWindow);
        GPUProfilerWindow.Reset();
    }
}

bool FRenderer::Initialize()
{
    CHECK(GInstance == nullptr);

    GInstance = new FRenderer();
    if (!GInstance->Create())
    {
        return false;
    }

    return true;
}

void FRenderer::Release()
{
    if (GInstance)
    {
        delete GInstance;
        GInstance = nullptr;
    }
}

FRenderer& FRenderer::Get()
{
    CHECK(GInstance != nullptr);
    return *GInstance;
}

bool FRenderer::Create()
{
    if (!GEngine->MainViewport)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.MainViewport = GEngine->MainViewport->GetRHIViewport();
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
        Resources.CameraBuffer->SetName("CameraBuffer");
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

    if (!DebugRenderer.Init(Resources))
    {
        return false;
    }

    if (!LightSetup.Init())
    {
        return false;
    }

    if (!DeferredRenderer.Init(Resources))
    {
        return false;
    }

    if (!ShadowMapRenderer.Init(LightSetup, Resources))
    {
        return false;
    }

    if (!SSAORenderer.Init(Resources))
    {
        return false;
    }

    if (!LightProbeRenderer.Init(LightSetup, Resources))
    {
        return false;
    }

    if (!SkyboxRenderPass.Init(Resources))
    {
        return false;
    }

    if (!ForwardRenderer.Init(Resources))
    {
        return false;
    }

    if (!TemporalAA.Init(Resources))
    {
        return false;
    }

    if (RHISupportsRayTracing())
    {
        if (!RayTracer.Init(Resources))
        {
            return false;
        }
    }

    LightProbeRenderer.RenderSkyLightProbe(CommandList, LightSetup, Resources);
    GRHICommandExecutor.ExecuteCommandList(CommandList);

    // Register EventFunc
    //GEngine->MainWindow->GetWindowResizedEvent().AddRaw(this, &FRenderer::OnWindowResize);

    //// Register Windows
    TextureDebugger = MakeShared<FRenderTargetDebugWindow>();
    //GEngine->MainWindow->AddOverlaySlot().AttachWidget(TextureDebugger);

    //InfoWindow = NewWidget(FRendererInfoWindow);
    //GEngine->MainWindow->AddOverlaySlot().AttachWidget(InfoWindow);

    //GPUProfilerWindow = NewWidget(FGPUProfilerWindow);
    //GEngine->MainWindow->AddOverlaySlot().AttachWidget(GPUProfilerWindow);
    return true;
}

void FRenderer::FrustumCullingAndSortingInternal(const FCamera* Camera, const TPair<uint32, uint32>& DrawCommands, TArray<uint32>& OutDeferredDrawCommands, TArray<uint32>& OutForwardDrawCommands)
{
    TRACE_SCOPE("Frustum Culling And Sorting Inner");

    // Inserts a mesh based on distance
    const auto InsertSorted = [](int32 CommandIndex, const FCamera* Camera, const FVector3& WorldPosition, TArray<float>& OutDistances, TArray<uint32>& OutCommands) -> void
    {
        CHECK(OutDistances.Size() == OutCommands.Size());

        FVector3 CameraPosition = Camera->GetPosition();
        FVector3 DistanceVector = WorldPosition - CameraPosition;

        const float NewDistance = DistanceVector.LengthSquared();

        int32 Index = 0;
        for (; Index < OutCommands.Size(); ++Index)
        {
            const float Distance = OutDistances[Index];
            if (NewDistance < Distance)
            {
                break;
            }
        }

        OutCommands.Insert(Index, CommandIndex);
        OutDistances.Insert(Index, NewDistance);
    };

    // Perform frustum culling and insert based on distance to the camera
    TArray<float> DeferredDistances;
    
    const uint32 StartCommand = DrawCommands.First;
    const uint32 NumCommands  = DrawCommands.Second;

    DeferredDistances.Reserve(NumCommands);
    OutDeferredDrawCommands.Reserve(NumCommands);

    const FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (uint32 Index = 0; Index < NumCommands; ++Index)
    {
        const uint32 CommandIndex = StartCommand + Index;

        const FMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];

        FMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        const FVector3 Top    = TransformMatrix.Transform(Command.Mesh->BoundingBox.Top);
        const FVector3 Bottom = TransformMatrix.Transform(Command.Mesh->BoundingBox.Bottom);

        FAABB Box(Top, Bottom);
        if (CameraFrustum.CheckAABB(Box))
        {
            if (Command.Material->ShouldRenderInForwardPass())
            {
                OutForwardDrawCommands.Emplace(CommandIndex);
            }
            else
            {
                FVector3 WorldPosition = Box.GetCenter();
                InsertSorted(CommandIndex, Camera, WorldPosition, DeferredDistances, OutDeferredDrawCommands);
            }
        }
    }
}

void FRenderer::PerformFrustumCullingAndSort(const FScene& Scene)
{
    TRACE_SCOPE("FrustumCulling And Sorting");

    const auto NumThreads = 1;// FPlatformThreadMisc::GetNumProcessors();
    
    TArray<TArray<uint32>> WriteableDeferredMeshCommands;
    WriteableDeferredMeshCommands.Reserve(NumThreads);

    TArray<TArray<uint32>> WriteableForwardMeshCommands;
    WriteableForwardMeshCommands.Reserve(NumThreads);

    TArray<TPair<uint32, uint32>> ReadableMeshCommands;
    ReadableMeshCommands.Reserve(NumThreads);

    const auto CameraPtr            = Scene.GetCamera();
    const auto NumMeshCommands      = Scene.GetMeshDrawCommands().Size();
    const auto NumCommandsPerThread = (NumMeshCommands / NumThreads) + 1;

    int32 RemainingCommands = NumMeshCommands;
    int32 StartCommand      = 0;

    TArray<FAsyncTaskBase*> Tasks(NumThreads);
    for (uint32 Index = 0; Index < NumThreads; ++Index)
    {
        // Allocate Array for commands to fill
        TArray<uint32>& WriteDeferredMeshCommands = WriteableDeferredMeshCommands.Emplace();
        TArray<uint32>& WriteForwardMeshCommands  = WriteableForwardMeshCommands.Emplace();
        
        const int32 NumCommands = FMath::Min<int32>(RemainingCommands, NumCommandsPerThread);
        RemainingCommands -= NumCommands;

        // Allocate ArrayView for reading
        TPair<uint32, uint32>& ReadMeshCommands = ReadableMeshCommands.Emplace(StartCommand, NumCommands);
        StartCommand += NumCommands;

        FAsyncTaskBase* AsyncTask = new TAsyncLambdaTask([&]() -> void
        {
            FrustumCullingAndSortingInternal(CameraPtr, ReadMeshCommands, WriteDeferredMeshCommands, WriteForwardMeshCommands);
        });

        AsyncTask->Launch();
        Tasks[Index] = AsyncTask;
    }

    // Sync and insert
    for (uint32 Index = 0; Index < NumThreads; ++Index)
    {
        FAsyncTaskBase* AsyncTask = Tasks[Index];
        AsyncTask->WaitForCompletion();
        delete AsyncTask;

        const TArray<uint32>& WriteForwardMeshCommands = WriteableForwardMeshCommands[Index];
        Resources.ForwardVisibleCommands.Append(WriteForwardMeshCommands);

        const TArray<uint32>& WriteDeferredMeshCommands = WriteableDeferredMeshCommands[Index];
        Resources.DeferredVisibleCommands.Append(WriteDeferredMeshCommands);
    }
}

void FRenderer::PerformFXAA(FRHICommandList& InCommandList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCommandList, "Begin FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(InCommandList, "FXAA");

    struct FFXAASettings
    {
        float Width;
        float Height;
    } Settings;

    Settings.Width  = static_cast<float>(Resources.BackBuffer->GetWidth());
    Settings.Height = static_cast<float>(Resources.BackBuffer->GetHeight());

    FRHIRenderPassDesc RenderPass;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    RenderPass.NumRenderTargets            = 1;

    InCommandList.BeginRenderPass(RenderPass);

    FRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    if (GFXAADebug.GetValue())
    {
        InCommandList.SetShaderResourceView(FXAADebugShader.Get(), FinalTargetSRV, 0);
        InCommandList.SetSamplerState(FXAADebugShader.Get(), Resources.FXAASampler.Get(), 0);
        InCommandList.Set32BitShaderConstants(FXAADebugShader.Get(), &Settings, 2);
        InCommandList.SetGraphicsPipelineState(FXAADebugPSO.Get());
    }
    else
    {
        InCommandList.SetShaderResourceView(FXAAShader.Get(), FinalTargetSRV, 0);
        InCommandList.SetSamplerState(FXAAShader.Get(), Resources.FXAASampler.Get(), 0);
        InCommandList.Set32BitShaderConstants(FXAAShader.Get(), &Settings, 2);
        InCommandList.SetGraphicsPipelineState(FXAAPSO.Get());
    }

    InCommandList.DrawInstanced(3, 1, 0, 0);

    InCommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(InCommandList, "End FXAA");
}

void FRenderer::PerformBackBufferBlit(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin Draw BackBuffer");

    TRACE_SCOPE("Draw to BackBuffer");

    FRHIRenderPassDesc RenderPass;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    RenderPass.NumRenderTargets            = 1;

    InCmdList.BeginRenderPass(RenderPass);

    FRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    InCmdList.SetShaderResourceView(PostShader.Get(), FinalTargetSRV, 0);
    InCmdList.SetSamplerState(PostShader.Get(), Resources.GBufferSampler.Get(), 0);

    InCmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);

    InCmdList.SetGraphicsPipelineState(PostPSO.Get());
    InCmdList.DrawInstanced(3, 1, 0, 0);

    InCmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End Draw BackBuffer");
}

void FRenderer::Tick(const FScene& Scene)
{
    GRHICommandExecutor.Tick();

    Resources.BackBuffer             = Resources.MainViewport->GetBackBuffer();
    Resources.GlobalMeshDrawCommands = TArrayView<const FMeshDrawCommand>(Scene.GetMeshDrawCommands());

    // Prepare Lights
#if 1
    CommandList.BeginExternalCapture();
#endif

    FGPUProfiler::Get().BeginGPUFrame(CommandList);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--BEGIN FRAME--");

    LightSetup.BeginFrame(CommandList, Scene);

    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();

    // Clear the images that were debug gable last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    // FrustumCulling
    if (!GFrustumCullEnabled.GetValue())
    {
        for (int32 CommandIndex = 0; CommandIndex < Resources.GlobalMeshDrawCommands.Size(); ++CommandIndex)
        {
            const FMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];
            if (Command.Material->ShouldRenderInForwardPass())
            {
                Resources.ForwardVisibleCommands.Emplace(CommandIndex);
            }
            else
            {
                Resources.DeferredVisibleCommands.Emplace(CommandIndex);
            }
        }
    }
    else
    {
        PerformFrustumCullingAndSort(Scene);
    }

    // Update camera-buffer
    // TODO: All matrices needs to be in Transposed the same
    CameraBuffer.PrevViewProjection = CameraBuffer.ViewProjection;
    
    CameraBuffer.ViewProjection     = Scene.GetCamera()->GetViewProjectionMatrix();
    CameraBuffer.ViewProjectionInv  = Scene.GetCamera()->GetViewProjectionInverseMatrix();

    CameraBuffer.ViewProjectionUnjittered    = CameraBuffer.ViewProjection;
    CameraBuffer.ViewProjectionInvUnjittered = CameraBuffer.ViewProjectionInv;

    CameraBuffer.View           = Scene.GetCamera()->GetViewMatrix();
    CameraBuffer.ViewInv        = Scene.GetCamera()->GetViewInverseMatrix();

    CameraBuffer.Projection     = Scene.GetCamera()->GetProjectionMatrix();
    CameraBuffer.ProjectionInv  = Scene.GetCamera()->GetProjectionInverseMatrix();

    CameraBuffer.Position       = Scene.GetCamera()->GetPosition();
    CameraBuffer.Forward        = Scene.GetCamera()->GetForward();
    CameraBuffer.Right          = Scene.GetCamera()->GetRight();
    CameraBuffer.NearPlane      = Scene.GetCamera()->GetNearPlane();
    CameraBuffer.FarPlane       = Scene.GetCamera()->GetFarPlane();
    CameraBuffer.AspectRatio    = Scene.GetCamera()->GetAspectRatio();
    CameraBuffer.ViewportWidth  = static_cast<float>(Resources.BackBuffer->GetWidth());
    CameraBuffer.ViewportHeight = static_cast<float>(Resources.BackBuffer->GetHeight());

    if (GEnableTemporalAA.GetValue())
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

    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(Resources.CameraBuffer.Get(), FBufferRegion(0, sizeof(FCameraBuffer)), &CameraBuffer);
    CommandList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Albedo].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Normal].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Material].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_ViewNormal].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Velocity].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    // PrePass
    if (GPrePassEnabled.GetValue())
    {
        DeferredRenderer.RenderPrePass(CommandList, Resources, Scene);
    }

    // Point Lights
    ShadowMapRenderer.RenderPointLightShadows(CommandList, LightSetup, Scene);

    // Directional Light
    ShadowMapRenderer.RenderDirectionalLightShadows(CommandList, LightSetup, Resources, Scene);

#if 0
    if (ShadingImage && GEnableVariableRateShading.GetValue())
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
    {
        DeferredRenderer.RenderBasePass(CommandList, Resources);
    }

    // RayTracing PrePass
    if (RHISupportsRayTracing())
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
    CommandList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    if (GEnableSSAO.GetValue())
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

    {
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionTexture(Resources.BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);
        CommandList.TransitionTexture(LightSetup.Skylight.IrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(LightSetup.Skylight.SpecularIrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

        ShadowMapRenderer.RenderShadowMasks(CommandList, LightSetup, Resources);

        DeferredRenderer.RenderDeferredTiledLightPass(CommandList, Resources, LightSetup);
    }

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::RenderTarget);

    SkyboxRenderPass.Render(CommandList, Resources, Scene);

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

    if (!Resources.ForwardVisibleCommands.IsEmpty())
    {
        GPU_TRACE_SCOPE(CommandList, "Forward Pass");
        ForwardRenderer.Render(CommandList, Resources, LightSetup);
    }

    if (GDrawPointLights.GetValue())
    {
        DebugRenderer.RenderPointLights(CommandList, Resources, Scene);
    }

    if (GDrawAABBs.GetValue())
    {
        DebugRenderer.RenderObjectAABBs(CommandList, Resources);
    }

    CommandList.TransitionTexture(Resources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView()),
        Resources.GBuffer[GBufferIndex_Depth],
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    if (GEnableTemporalAA.GetValue())
    {
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::RenderTarget, EResourceAccess::UnorderedAccess);

        TemporalAA.Render(CommandList, Resources);

        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
    }
    else
    {
        CommandList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource);
    }

    if (GEnableFXAA.GetValue())
    {
        PerformFXAA(CommandList);
    }
    else
    {
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

#if 0
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

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--END FRAME--");

    FGPUProfiler::Get().EndGPUFrame(CommandList);

#if 1
    CommandList.EndExternalCapture();
#endif

    CommandList.PresentViewport(Resources.MainViewport.Get(), GVSyncEnabled.GetValue());

    {
        TRACE_SCOPE("ExecuteCommandList");

        GRHICommandExecutor.WaitForOutstandingTasks();
        GRHICommandExecutor.ExecuteCommandList(CommandList);
        FrameStatistics = GRHICommandExecutor.GetStatistics();
    }
}

void FRenderer::OnWindowResize(const FWindowEvent& Event)
{
    const uint32 Width  = Event.GetWidth();
    const uint32 Height = Event.GetHeight();

    if (Width > 0 && Height > 0)
    {
        GRHICommandExecutor.WaitForOutstandingTasks();

        if (!Resources.MainViewport->Resize(Width, Height))
        {
            DEBUG_BREAK();
            return;
        }

        if (!DeferredRenderer.ResizeResources(Resources))
        {
            DEBUG_BREAK();
            return;
        }

        if (!SSAORenderer.ResizeResources(Resources))
        {
            DEBUG_BREAK();
            return;
        }

        if (!ShadowMapRenderer.ResizeResources(Width, Height, LightSetup))
        {
            DEBUG_BREAK();
            return;
        }

        if (!TemporalAA.ResizeResources(Resources))
        {
            DEBUG_BREAK();
            return;
        }
    }
}

bool FRenderer::InitAA()
{
    TArray<uint8> ShaderCode;
    
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
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
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/PostProcessPS.hlsl", CompileInfo, ShaderCode))
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

    FRHIGraphicsPipelineStateDesc PSOInitializer;
    PSOInitializer.VertexInputLayout                      = nullptr;
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = VShader.Get();
    PSOInitializer.ShaderState.PixelShader                = PostShader.Get();
    PSOInitializer.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
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
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
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
        FXAAPSO->SetName("FXAA PipelineState");
    }

    TArray<FShaderDefine> Defines =
    {
        FShaderDefine("ENABLE_DEBUG", "(1)")
    };

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel, MakeArrayView(Defines));
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
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

bool FRenderer::InitShadingImage()
{
    FRHIShadingRateSupport Support;
    RHIQueryShadingRateSupport(Support);

    if (Support.Tier != ERHIShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.MainViewport->GetWidth() / Support.ShadingRateImageTileSize;
    const uint32 Height = Resources.MainViewport->GetHeight() / Support.ShadingRateImageTileSize;

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    ShadingImage = RHICreateTexture(TextureDesc, EResourceAccess::ShadingRateSource);
    if (!ShadingImage)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadingImage->SetName("Shading Rate Image");
    }

    TArray<uint8> ShaderCode;
    
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadingImage.hlsl", CompileInfo, ShaderCode))
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

    FRHIComputePipelineStateDesc PSOInitializer(ShadingRateShader.Get());
    ShadingRatePipeline = RHICreateComputePipelineState(PSOInitializer);
    if (!ShadingRatePipeline)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}
