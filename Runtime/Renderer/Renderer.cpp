#include "Renderer.h"

#include "Canvas/CanvasApplication.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/TextureFactory.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Engine.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Frustum.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variables

TAutoConsoleVariable<bool> GEnableSSAO("Renderer.EnableSSAO", true);

TAutoConsoleVariable<bool> GEnableFXAA("Renderer.EnableFXAA", true);
TAutoConsoleVariable<bool> GFXAADebug("Renderer.FXAADebug", false);

TAutoConsoleVariable<bool> GEnableVariableRateShading("Renderer.EnableVariableRateShading", false);

TAutoConsoleVariable<bool> GPrePassEnabled("Renderer.EnablePrePass", true);
TAutoConsoleVariable<bool> GDrawAABBs("Renderer.EnableDrawAABBs", false);
TAutoConsoleVariable<bool> GVSyncEnabled("Renderer.EnableVerticalSync", false);
TAutoConsoleVariable<bool> GFrustumCullEnabled("Renderer.EnableFrustumCulling", true);
TAutoConsoleVariable<bool> GRayTracingEnabled("Renderer.EnableRayTracing", true);

//static const uint32 ShadowMapSampleCount = 2;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCameraBufferDesc

struct SCameraBufferDesc
{
    FMatrix4 ViewProjection;
    FMatrix4 View;
    FMatrix4 ViewInv;
    FMatrix4 Projection;
    FMatrix4 ProjectionInv;
    FMatrix4 ViewProjectionInv;

    FVector3 Position;
    float    NearPlane;

    FVector3 Forward;
    float    FarPlane;

    FVector3 Right;
    float    AspectRatio;
};

RENDERER_API CRenderer GRenderer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRenderer

CRenderer::CRenderer()
    : WindowHandler(MakeShared<CRendererWindowHandler>())
{ }

bool CRenderer::Init()
{
    FRHIViewportInitializer ViewportInitializer( GEngine->MainWindow->GetPlatformHandle()
                                               , EFormat::R8G8B8A8_Unorm
                                               , EFormat::Unknown
                                               , GEngine->MainWindow->GetWidth()
                                               , GEngine->MainWindow->GetHeight());

    Resources.MainWindowViewport = RHICreateViewport(ViewportInitializer);
    if (!Resources.MainWindowViewport)
    {
        FDebug::DebugBreak();
        return false;
    }
    else
    {
        // Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    FRHIConstantBufferInitializer CBInitializer(EBufferUsageFlags::Default, sizeof(SCameraBufferDesc), EResourceAccess::Common);
    Resources.CameraBuffer = RHICreateConstantBuffer(CBInitializer);
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
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(SVertex), 0, 0,  EVertexInputClass::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, sizeof(SVertex), 0, 12, EVertexInputClass::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, sizeof(SVertex), 0, 24, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(SVertex), 0, 36, EVertexInputClass::Vertex, 0 },
    };

    Resources.StdInputLayout = RHICreateVertexInputLayout(InputLayout);
    if (!Resources.StdInputLayout)
    {
        FDebug::DebugBreak();
        return false;
    }

    {
        FRHISamplerStateInitializer Initializer;
        Initializer.AddressU    = ESamplerMode::Border;
        Initializer.AddressV    = ESamplerMode::Border;
        Initializer.AddressW    = ESamplerMode::Border;
        Initializer.Filter      = ESamplerFilter::MinMagMipPoint;
        Initializer.BorderColor = FFloatColor(1.0f, 1.0f, 1.0f, 1.0f);

        Resources.DirectionalLightShadowSampler = RHICreateSamplerState(Initializer);
        if (!Resources.DirectionalLightShadowSampler)
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    {
        FRHISamplerStateInitializer Initializer;
        Initializer.AddressU       = ESamplerMode::Wrap;
        Initializer.AddressV       = ESamplerMode::Wrap;
        Initializer.AddressW       = ESamplerMode::Wrap;
        Initializer.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        Initializer.ComparisonFunc = EComparisonFunc::LessEqual;

        Resources.PointLightShadowSampler = RHICreateSamplerState(Initializer);
        if (!Resources.PointLightShadowSampler)
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    if (!InitAA())
    {
        return false;
    }

    if (!InitBoundingBoxDebugPass())
    {
        return false;
    }

    if (!InitShadingImage())
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

    if (RHISupportsRayTracing())
    {
        if (!RayTracer.Init(Resources))
        {
            return false;
        }
    }

    LightProbeRenderer.RenderSkyLightProbe(MainCmdList, LightSetup, Resources);

    FRHICommandQueue::Get().ExecuteCommandList(MainCmdList);

    CCanvasApplication& Application = CCanvasApplication::Get();

    // Register EventFunc
    WindowHandler->WindowResizedDelegate.BindRaw(this, &CRenderer::OnWindowResize);
    Application.AddWindowMessageHandler(WindowHandler, uint32(-1));

    // Register Windows
    TextureDebugger = CTextureDebugWindow::Make();
    Application.AddWindow(TextureDebugger);

    InfoWindow = CRendererInfoWindow::Make();
    Application.AddWindow(InfoWindow);

    GPUProfilerWindow = CGPUProfilerWindow::Make();
    Application.AddWindow(GPUProfilerWindow);

    return true;
}

void CRenderer::FrustumCullingAndSortingInternal( const CCamera* Camera
                                                , const TPair<uint32, uint32>& DrawCommands
                                                , TArray<uint32>& OutDeferredDrawCommands
                                                , TArray<uint32>& OutForwardDrawCommands)
{

    TRACE_SCOPE("Frustum Culling And Sorting Inner");

    // Inserts a mesh based on distance
    const auto InsertSorted = []( int32 CommandIndex
                                , const CCamera* Camera
                                , const FVector3& WorldPosition
                                , TArray<float>& OutDistances
                                , TArray<uint32>& OutCommands) -> void
    {
        Check(OutDistances.Size() == OutCommands.Size());

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

    FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (uint32 Index = 0; Index < NumCommands; ++Index)
    {
        const uint32 CommandIndex = StartCommand + Index;

        const SMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];

        FMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        FVector3 Top = FVector3(Command.Mesh->BoundingBox.Top);
        Top = TransformMatrix.TransformPosition(Top);

        FVector3 Bottom = FVector3(Command.Mesh->BoundingBox.Bottom);
        Bottom = TransformMatrix.TransformPosition(Bottom);

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

void CRenderer::PerformFrustumCullingAndSort(const CScene& Scene)
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

    TArray<DispatchID> Tasks(NumThreads);
    for (uint32 Index = 0; Index < NumThreads; ++Index)
    {
        // Allocate Array for commands to fill
        TArray<uint32>& WriteDeferredMeshCommands = WriteableDeferredMeshCommands.Emplace();
        TArray<uint32>& WriteForwardMeshCommands  = WriteableForwardMeshCommands.Emplace();
        
        const int32 NumCommands = NMath::Min<int32>(RemainingCommands, NumCommandsPerThread);
        RemainingCommands -= NumCommands;

        // Allocate ArrayView for reading
        TPair<uint32, uint32>& ReadMeshCommands = ReadableMeshCommands.Emplace(StartCommand, NumCommands);
        StartCommand += NumCommands;

        const auto CullAndSort = [&]() -> void
        {
            FrustumCullingAndSortingInternal(CameraPtr, ReadMeshCommands, WriteDeferredMeshCommands, WriteForwardMeshCommands);
        };

        FAsyncTask AsyncTask;
        AsyncTask.Delegate.BindLambda(CullAndSort);

        Tasks[Index] = FAsyncTaskManager::Get().Dispatch(AsyncTask);
    }

    // Sync and insert
    for (uint32 Index = 0; Index < NumThreads; ++Index)
    {
        FAsyncTaskManager::Get().WaitFor(Tasks[Index], true);
        
        const TArray<uint32>& WriteForwardMeshCommands = WriteableForwardMeshCommands[Index];
        Resources.ForwardVisibleCommands.Append(WriteForwardMeshCommands);

        const TArray<uint32>& WriteDeferredMeshCommands = WriteableDeferredMeshCommands[Index];
        Resources.DeferredVisibleCommands.Append(WriteDeferredMeshCommands);
    }
}

void CRenderer::PerformFXAA(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(InCmdList, "FXAA");

    struct SFXAASettings
    {
        float Width;
        float Height;
    } Settings;

    Settings.Width  = static_cast<float>(Resources.BackBuffer->GetWidth());
    Settings.Height = static_cast<float>(Resources.BackBuffer->GetHeight());

    FRHIRenderPassInitializer RenderPass;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    RenderPass.NumRenderTargets            = 1;

    InCmdList.BeginRenderPass(RenderPass);

    FRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    if (GFXAADebug.GetBool())
    {
        InCmdList.SetShaderResourceView(FXAADebugShader.Get(), FinalTargetSRV, 0);
        InCmdList.SetSamplerState(FXAADebugShader.Get(), Resources.FXAASampler.Get(), 0);
        InCmdList.Set32BitShaderConstants(FXAADebugShader.Get(), &Settings, 2);
        InCmdList.SetGraphicsPipelineState(FXAADebugPSO.Get());
    }
    else
    {
        InCmdList.SetShaderResourceView(FXAAShader.Get(), FinalTargetSRV, 0);
        InCmdList.SetSamplerState(FXAAShader.Get(), Resources.FXAASampler.Get(), 0);
        InCmdList.Set32BitShaderConstants(FXAAShader.Get(), &Settings, 2);
        InCmdList.SetGraphicsPipelineState(FXAAPSO.Get());
    }

    InCmdList.DrawInstanced(3, 1, 0, 0);

    InCmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End FXAA");
}

void CRenderer::PerformBackBufferBlit(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin Draw BackBuffer");

    TRACE_SCOPE("Draw to BackBuffer");

    FRHIRenderPassInitializer RenderPass;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    RenderPass.NumRenderTargets            = 1;

    InCmdList.BeginRenderPass(RenderPass);

    FRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    InCmdList.SetShaderResourceView(PostShader.Get(), FinalTargetSRV, 0);
    InCmdList.SetSamplerState(PostShader.Get(), Resources.GBufferSampler.Get(), 0);

    InCmdList.SetGraphicsPipelineState(PostPSO.Get());
    InCmdList.DrawInstanced(3, 1, 0, 0);

    InCmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End Draw BackBuffer");
}

void CRenderer::PerformAABBDebugPass(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin DebugPass");

    TRACE_SCOPE("DebugPass");

    InCmdList.SetGraphicsPipelineState(AABBDebugPipelineState.Get());

    InCmdList.SetPrimitiveTopology(EPrimitiveTopology::LineList);

    InCmdList.SetConstantBuffer(AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0);

    InCmdList.SetVertexBuffers(&AABBVertexBuffer, 1, 0);
    InCmdList.SetIndexBuffer(AABBIndexBuffer.Get());

    for (const auto CommandIndex : Resources.DeferredVisibleCommands)
    {
        const SMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];

        FAABB& Box = Command.Mesh->BoundingBox;

        FVector3 Scale    = FVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        FVector3 Position = Box.GetCenter();

        FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.x, Position.y, Position.z);
        FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.x, Scale.y, Scale.z);
        FMatrix4 TransformMatrix   = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();
        TransformMatrix = (ScaleMatrix * TranslationMatrix) * TransformMatrix;
        TransformMatrix.Transpose();

        InCmdList.Set32BitShaderConstants(AABBVertexShader.Get(), &TranslationMatrix, 16);

        InCmdList.DrawIndexedInstanced(24, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End DebugPass");
}

void CRenderer::Tick(const CScene& Scene)
{
    Resources.BackBuffer             = Resources.MainWindowViewport->GetBackBuffer();
    Resources.GlobalMeshDrawCommands = TArrayView<const SMeshDrawCommand>(Scene.GetMeshDrawCommands());

    // Prepare Lights
#if 1
    PreShadowsCmdList.BeginExternalCapture();
#endif

    CGPUProfiler::Get().BeginGPUFrame(PreShadowsCmdList);

    INSERT_DEBUG_CMDLIST_MARKER(PreShadowsCmdList, "--BEGIN FRAME--");

    LightSetup.BeginFrame(PreShadowsCmdList, Scene);

    // Initialize point light task
    const auto RenderPointShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderPointLightShadows(PointShadowCmdList, LightSetup, Scene);
    };

    if (!PointShadowTask.Delegate.IsBound())
    {
        PointShadowTask.Delegate.BindLambda(RenderPointShadows);
    }

    FAsyncTaskManager::Get().Dispatch(PointShadowTask);

    // Initialize directional light task
    const auto RenderDirShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderDirectionalLightShadows(DirShadowCmdList, LightSetup, Resources, Scene);
    };

    DirShadowTask.Delegate.BindLambda(RenderDirShadows);
    FAsyncTaskManager::Get().Dispatch(DirShadowTask);

    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();

    // Clear the images that were debug gable last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    if (!GFrustumCullEnabled.GetBool())
    {
        for (int32 CommandIndex = 0; CommandIndex < Resources.GlobalMeshDrawCommands.Size(); ++CommandIndex)
        {
            const SMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];
            if (Command.Material->HasAlphaMask())
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
    SCameraBufferDesc CamBuffer;
    CamBuffer.ViewProjection    = Scene.GetCamera()->GetViewProjectionMatrix();
    CamBuffer.View              = Scene.GetCamera()->GetViewMatrix();
    CamBuffer.ViewInv           = Scene.GetCamera()->GetViewInverseMatrix();
    CamBuffer.Projection        = Scene.GetCamera()->GetProjectionMatrix();
    CamBuffer.ProjectionInv     = Scene.GetCamera()->GetProjectionInverseMatrix();
    CamBuffer.ViewProjectionInv = Scene.GetCamera()->GetViewProjectionInverseMatrix();
    CamBuffer.Position          = Scene.GetCamera()->GetPosition();
    CamBuffer.Forward           = Scene.GetCamera()->GetForward();
    CamBuffer.Right             = Scene.GetCamera()->GetRight();
    CamBuffer.NearPlane         = Scene.GetCamera()->GetNearPlane();
    CamBuffer.FarPlane          = Scene.GetCamera()->GetFarPlane();
    CamBuffer.AspectRatio       = Scene.GetCamera()->GetAspectRatio();

    PrepareGBufferCmdList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);

    PrepareGBufferCmdList.UpdateBuffer(Resources.CameraBuffer.Get(), 0, sizeof(SCameraBufferDesc), &CamBuffer);

    PrepareGBufferCmdList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    if (GPrePassEnabled.GetBool())
    {
        const auto RenderPrePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderPrePass(PrePassCmdList, Resources, Scene);
        };

        PrePassTask.Delegate.BindLambda(RenderPrePass);
        FAsyncTaskManager::Get().Dispatch(PrePassTask);
    }

#if 0
    if (ShadingImage && GEnableVariableRateShading.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(ShadingRateCmdList, "Begin VRS Image");
        ShadingRateCmdList.SetShadingRate(EShadingRate::VRS_1x1);

        ShadingRateCmdList.TransitionTexture(ShadingImage.Get(), EResourceAccess::ShadingRateSource, EResourceAccess::UnorderedAccess);

        ShadingRateCmdList.SetComputePipelineState(ShadingRatePipeline.Get());

        FRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        ShadingRateCmdList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

        ShadingRateCmdList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);

        ShadingRateCmdList.TransitionTexture(ShadingImage.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::ShadingRateSource);

        ShadingRateCmdList.SetShadingRateImage(ShadingImage.Get());

        INSERT_DEBUG_CMDLIST_MARKER(ShadingRateCmdList, "End VRS Image");
    }
    else if (RHISupportsVariableRateShading())
    {
        ShadingRateCmdList.SetShadingRate(EShadingRate::VRS_1x1);
    }
#endif

    if (RHISupportsRayTracing())
    {
        const auto RenderRayTracing = [&]()
        {
            GPU_TRACE_SCOPE(RayTracingCmdList, "Ray Tracing");
            CRenderer::RayTracer.PreRender(RayTracingCmdList, Resources, Scene);
        };

        RayTracingTask.Delegate.BindLambda(RenderRayTracing);
        FAsyncTaskManager::Get().Dispatch(RayTracingTask);
    }

    {
        const auto RenderBasePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderBasePass(BasePassCmdList, Resources);
        };

        BasePassTask.Delegate.BindLambda(RenderBasePass);
        FAsyncTaskManager::Get().Dispatch(BasePassTask);
    }

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView())
                   , Resources.GBuffer[GBUFFER_ALBEDO_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView())
                   , Resources.GBuffer[GBUFFER_NORMAL_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView())
                   , Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView())
                   , Resources.GBuffer[GBUFFER_MATERIAL_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    MainCmdList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    MainCmdList.ClearUnorderedAccessView(Resources.SSAOBuffer->GetUnorderedAccessView(), { 1.0f, 1.0f, 1.0f, 1.0f });

    if (GEnableSSAO.GetBool())
    {
        GPU_TRACE_SCOPE(MainCmdList, "SSAO");
        SSAORenderer.Render(MainCmdList, Resources);
    }

    MainCmdList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.SSAOBuffer->GetShaderResourceView())
                   , Resources.SSAOBuffer
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    {
        MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess);
        MainCmdList.TransitionTexture(Resources.BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);
        MainCmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        MainCmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        MainCmdList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

        ShadowMapRenderer.RenderShadowMasks(MainCmdList, LightSetup, Resources);

        DeferredRenderer.RenderDeferredTiledLightPass(MainCmdList, Resources, LightSetup);
    }

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::RenderTarget);

    SkyboxRenderPass.Render(MainCmdList, Resources, Scene);

    MainCmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(LightSetup.DirectionalShadowMask->GetShaderResourceView())
                   , LightSetup.DirectionalShadowMask
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[0]->GetShaderResourceView())
                   , LightSetup.ShadowMapCascades[0]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[1]->GetShaderResourceView())
                   , LightSetup.ShadowMapCascades[1]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[2]->GetShaderResourceView())
                   , LightSetup.ShadowMapCascades[2]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(LightSetup.ShadowMapCascades[3]->GetShaderResourceView())
                   , LightSetup.ShadowMapCascades[3]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    MainCmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    MainCmdList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.IntegrationLUT->GetShaderResourceView())
                   , Resources.IntegrationLUT
                   , EResourceAccess::PixelShaderResource
                   , EResourceAccess::PixelShaderResource);

    if (!Resources.ForwardVisibleCommands.IsEmpty())
    {
        GPU_TRACE_SCOPE(MainCmdList, "Forward Pass");
        ForwardRenderer.Render(MainCmdList, Resources, LightSetup);
    }

    MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.FinalTarget->GetShaderResourceView())
                   , Resources.FinalTarget
                   , EResourceAccess::PixelShaderResource
                   , EResourceAccess::PixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView())
                   , Resources.GBuffer[GBUFFER_DEPTH_INDEX]
                   , EResourceAccess::PixelShaderResource
                   , EResourceAccess::PixelShaderResource);

    if (GEnableFXAA.GetBool())
    {
        PerformFXAA(MainCmdList);
    }
    else
    {
        PerformBackBufferBlit(MainCmdList);
    }

    if (GDrawAABBs.GetBool())
    {
        PerformAABBDebugPass(MainCmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "Begin UI Render");

    {
        TRACE_SCOPE("Render UI");

#if 0
        if (RHISupportsVariableRateShading())
        {
            MainCmdList.SetShadingRate(EShadingRate::VRS_1x1);
            MainCmdList.SetShadingRateImage(nullptr);
        }
#endif

        FRHIRenderPassInitializer RenderPass;
        RenderPass.RenderTargets[0] = FRHIRenderTargetView(Resources.BackBuffer, EAttachmentLoadAction::Load);
        RenderPass.NumRenderTargets = 1;

        MainCmdList.BeginRenderPass(RenderPass);
        
        CCanvasApplication::Get().DrawWindows(MainCmdList);
        
        MainCmdList.EndRenderPass();
    }

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "End UI Render");

    MainCmdList.TransitionTexture(Resources.BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "--END FRAME--");

    CGPUProfiler::Get().EndGPUFrame(MainCmdList);

#if 1
    MainCmdList.EndExternalCapture();
#endif

    FAsyncTaskManager::Get().WaitForAll();

    {
        TRACE_SCOPE("ExecuteCommandList");

        FRHICommandList* CmdLists[9] =
        {
            &PreShadowsCmdList,
            &PointShadowCmdList,
            &DirShadowCmdList,
            &PrepareGBufferCmdList,
            &PrePassCmdList,
            &ShadingRateCmdList,
            &RayTracingCmdList,
            &BasePassCmdList,
            &MainCmdList
        };

        FRHICommandQueue::Get().ExecuteCommandLists(CmdLists, ArrayCount(CmdLists));

        FrameStatistics.NumDrawCalls      = FRHICommandQueue::Get().GetNumDrawCalls();
        FrameStatistics.NumDispatchCalls  = FRHICommandQueue::Get().GetNumDispatchCalls();
        FrameStatistics.NumRenderCommands = FRHICommandQueue::Get().GetNumCommands();
    }

    {
        TRACE_SCOPE("Present");
        Resources.MainWindowViewport->Present(GVSyncEnabled.GetBool());
    }
}

void CRenderer::Release()
{
    FRHICommandQueue::Get().WaitForGPU();

    PreShadowsCmdList.Reset();
    PointShadowCmdList.Reset();
    DirShadowCmdList.Reset();
    PrepareGBufferCmdList.Reset();
    PrePassCmdList.Reset();
    ShadingRateCmdList.Reset();
    RayTracingCmdList.Reset();
    BasePassCmdList.Reset();
    MainCmdList.Reset();

    DeferredRenderer.Release();
    ShadowMapRenderer.Release();
    SSAORenderer.Release();
    LightProbeRenderer.Release();
    SkyboxRenderPass.Release();
    ForwardRenderer.Release();
    RayTracer.Release();

    Resources.Release();
    LightSetup.Release();

    AABBVertexBuffer.Reset();
    AABBIndexBuffer.Reset();
    AABBDebugPipelineState.Reset();
    AABBVertexShader.Reset();
    AABBPixelShader.Reset();

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

    if (CCanvasApplication::IsInitialized())
    {
        CCanvasApplication& Application = CCanvasApplication::Get();
        Application.RemoveWindow(TextureDebugger);
        TextureDebugger.Reset();

        Application.RemoveWindow(InfoWindow);
        InfoWindow.Reset();

        Application.RemoveWindow(GPUProfilerWindow);
        GPUProfilerWindow.Reset();
    }
}

void CRenderer::OnWindowResize(const SWindowResizeEvent& Event)
{
    const uint32 Width = Event.Width;
    const uint32 Height = Event.Height;

    if (!Resources.MainWindowViewport->Resize(Width, Height))
    {
        FDebug::DebugBreak();
        return;
    }

    if (!DeferredRenderer.ResizeResources(Resources))
    {
        FDebug::DebugBreak();
        return;
    }

    if (!SSAORenderer.ResizeResources(Resources))
    {
        FDebug::DebugBreak();
        return;
    }

    if (!ShadowMapRenderer.ResizeResources(Width, Height, LightSetup))
    {
        FDebug::DebugBreak();
        return;
    }
}

bool CRenderer::InitBoundingBoxDebugPass()
{
    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    AABBVertexShader = RHICreateVertexShader(ShaderCode);
    if (!AABBVertexShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    AABBPixelShader = RHICreatePixelShader(ShaderCode);
    if (!AABBPixelShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIVertexInputLayoutInitializer InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 },
    };

    TSharedRef<FRHIVertexInputLayout> InputLayoutState = RHICreateVertexInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilInitializer.bDepthEnable   = false;
    DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<FRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<FRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInfo;

    TSharedRef<FRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.VertexInputLayout                      = InputLayoutState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = AABBVertexShader.Get();
    PSOInitializer.ShaderState.PixelShader                = AABBPixelShader.Get();
    PSOInitializer.PrimitiveTopologyType                  = EPrimitiveTopologyType::Line;
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = Resources.RenderTargetFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = Resources.DepthBufferFormat;

    AABBDebugPipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!AABBDebugPipelineState)
    {
        FDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBDebugPipelineState->SetName("Debug PipelineState");
    }

    TStaticArray<FVector3, 8> Vertices =
    {
        FVector3(-0.5f, -0.5f,  0.5f),
        FVector3( 0.5f, -0.5f,  0.5f),
        FVector3(-0.5f,  0.5f,  0.5f),
        FVector3( 0.5f,  0.5f,  0.5f),

        FVector3( 0.5f, -0.5f, -0.5f),
        FVector3(-0.5f, -0.5f, -0.5f),
        FVector3( 0.5f,  0.5f, -0.5f),
        FVector3(-0.5f,  0.5f, -0.5f)
    };

    FRHIBufferDataInitializer VertexData(Vertices.Data(), Vertices.SizeInBytes());

    FRHIVertexBufferInitializer VBInitializer(EBufferUsageFlags::Default, Vertices.Size(), sizeof(FVector3), EResourceAccess::Common, &VertexData);
    AABBVertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!AABBVertexBuffer)
    {
        FDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBVertexBuffer->SetName("AABB VertexBuffer");
    }

    // Create IndexBuffer
    TStaticArray<uint16, 24> Indices =
    {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        1, 4,
        3, 6,
        6, 4,
        4, 5,
        5, 7,
        7, 6,
        0, 5,
        2, 7,
    };

    FRHIBufferDataInitializer IndexData(Indices.Data(), Indices.SizeInBytes());

    FRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, EIndexFormat::uint16, Indices.Size(), EResourceAccess::Common, &IndexData);
    AABBIndexBuffer = RHICreateIndexBuffer(IBInitializer);
    if (!AABBIndexBuffer)
    {
        FDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBIndexBuffer->SetName("AABB IndexBuffer");
    }

    return true;
}

bool CRenderer::InitAA()
{
    TArray<uint8> ShaderCode;
    
    {
		FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Vertex);
		if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
		{
			FDebug::DebugBreak();
			return false;
		}
    }

    TSharedRef<FRHIVertexShader> VShader = RHICreateVertexShader(ShaderCode);
    if (!VShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    {
		FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel);
		if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/PostProcessPS.hlsl", CompileInfo, ShaderCode))
		{
			FDebug::DebugBreak();
			return false;
		}
    }

    PostShader = RHICreatePixelShader(ShaderCode);
    if (!PostShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc      = EComparisonFunc::Always;
    DepthStencilInitializer.bDepthEnable   = false;
    DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<FRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    TSharedRef<FRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInfo;

    TSharedRef<FRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
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
        FDebug::DebugBreak();
        return false;
    }

    // FXAA
    FRHISamplerStateInitializer SamplerInitializer;
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
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    FXAAShader = RHICreatePixelShader(ShaderCode);
    if (!FXAAShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    PSOInitializer.ShaderState.PixelShader = FXAAShader.Get();

    FXAAPSO = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!FXAAPSO)
    {
        FDebug::DebugBreak();
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
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel, Defines.CreateView());
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    FXAADebugShader = RHICreatePixelShader(ShaderCode);
    if (!FXAADebugShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    PSOInitializer.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!FXAADebugPSO)
    {
        FDebug::DebugBreak();
        return false;
    }

    return true;
}

bool CRenderer::InitShadingImage()
{
    FShadingRateSupport Support;
    RHIQueryShadingRateSupport(Support);

    if (Support.Tier != ERHIShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.MainWindowViewport->GetWidth() / Support.ShadingRateImageTileSize;
    const uint32 Height = Resources.MainWindowViewport->GetHeight() / Support.ShadingRateImageTileSize;

    FRHITexture2DInitializer Initializer(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::RWTexture, EResourceAccess::ShadingRateSource);
    ShadingImage = RHICreateTexture2D(Initializer);
    if (!ShadingImage)
    {
        FDebug::DebugBreak();
        return false;
    }
    else
    {
        ShadingImage->SetName("Shading Rate Image");
    }

    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadingImage.hlsl", CompileInfo, ShaderCode))
        {
            FDebug::DebugBreak();
            return false;
        }
    }

    ShadingRateShader = RHICreateComputeShader(ShaderCode);
    if (!ShadingRateShader)
    {
        FDebug::DebugBreak();
        return false;
    }

    FRHIComputePipelineStateInitializer PSOInitializer(ShadingRateShader.Get());
    ShadingRatePipeline = RHICreateComputePipelineState(PSOInitializer);
    if (!ShadingRatePipeline)
    {
        FDebug::DebugBreak();
        return false;
    }

    return true;
}
