#include "Renderer.h"

#include "Application/ApplicationInstance.h"

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
    CMatrix4 ViewProjection;
    CMatrix4 View;
    CMatrix4 ViewInv;
    CMatrix4 Projection;
    CMatrix4 ProjectionInv;
    CMatrix4 ViewProjectionInv;

    CVector3 Position;
    float    NearPlane;

    CVector3 Forward;
    float    FarPlane;

    CVector3 Right;
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
    CRHIViewportInitializer ViewportInitializer(GEngine->MainWindow->GetPlatformHandle(), EFormat::R8G8B8A8_Unorm, EFormat::Unknown, 0, 0);

    Resources.MainWindowViewport = RHICreateViewport(ViewportInitializer);
    if (!Resources.MainWindowViewport)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        // Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    CRHIConstantBufferInitializer CBInitializer(EBufferUsageFlags::Default, sizeof(SCameraBufferDesc), EResourceAccess::Common);
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

    // Init standard input layout
    CRHIVertexInputLayoutInitializer InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, 0, 0,  EVertexInputClass::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, 0, 12, EVertexInputClass::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, 0, 24, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    0, 36, EVertexInputClass::Vertex, 0 },
    };

    Resources.StdInputLayout = RHICreateVertexInputLayout(InputLayout);
    if (!Resources.StdInputLayout)
    {
        CDebug::DebugBreak();
        return false;
    }

    {
        CRHISamplerStateInitializer Initializer;
        Initializer.AddressU    = ESamplerMode::Border;
        Initializer.AddressV    = ESamplerMode::Border;
        Initializer.AddressW    = ESamplerMode::Border;
        Initializer.Filter      = ESamplerFilter::MinMagMipPoint;
        Initializer.BorderColor = CFloatColor(1.0f, 1.0f, 1.0f, 1.0f);

        Resources.DirectionalLightShadowSampler = RHICreateSamplerState(Initializer);
        if (!Resources.DirectionalLightShadowSampler)
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    {
        CRHISamplerStateInitializer Initializer;
        Initializer.AddressU       = ESamplerMode::Wrap;
        Initializer.AddressV       = ESamplerMode::Wrap;
        Initializer.AddressW       = ESamplerMode::Wrap;
        Initializer.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        Initializer.ComparisonFunc = EComparisonFunc::LessEqual;

        Resources.PointLightShadowSampler = RHICreateSamplerState(Initializer);
        if (!Resources.PointLightShadowSampler)
        {
            CDebug::DebugBreak();
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

    CRHICommandQueue::Get().ExecuteCommandList(MainCmdList);

    CApplicationInstance& Application = CApplicationInstance::Get();

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
                                , const CVector3& WorldPosition
                                , TArray<float>& OutDistances
                                , TArray<uint32>& OutCommands) -> void
    {
        Assert(OutDistances.Size() == OutCommands.Size());

        CVector3 CameraPosition = Camera->GetPosition();
        CVector3 DistanceVector = WorldPosition - CameraPosition;

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

    CFrustum CameraFrustum = CFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (uint32 Index = 0; Index < NumCommands; ++Index)
    {
        const uint32 CommandIndex = StartCommand + Index;

        const SMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];

        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        CVector3 Top = CVector3(Command.Mesh->BoundingBox.Top);
        Top = TransformMatrix.TransformPosition(Top);

        CVector3 Bottom = CVector3(Command.Mesh->BoundingBox.Bottom);
        Bottom = TransformMatrix.TransformPosition(Bottom);

        SAABB Box(Top, Bottom);
        if (CameraFrustum.CheckAABB(Box))
        {
            if (Command.Material->ShouldRenderInForwardPass())
            {
                OutForwardDrawCommands.Emplace(CommandIndex);
            }
            else
            {
                CVector3 WorldPosition = Box.GetCenter();
                InsertSorted(CommandIndex, Camera, WorldPosition, DeferredDistances, OutDeferredDrawCommands);
            }
        }
    }
}

void CRenderer::PerformFrustumCullingAndSort(const CScene& Scene)
{
    TRACE_SCOPE("FrustumCulling And Sorting");

    const auto NumThreads = 1;// PlatformThreadMisc::GetNumProcessors();
    
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

        SAsyncTask AsyncTask;
        AsyncTask.Delegate.BindLambda(CullAndSort);

        Tasks[Index] = CAsyncTaskManager::Get().Dispatch(AsyncTask);
    }

    // Sync and insert
    for (uint32 Index = 0; Index < NumThreads; ++Index)
    {
        CAsyncTaskManager::Get().WaitFor(Tasks[Index], true);
        
        const TArray<uint32>& WriteForwardMeshCommands = WriteableForwardMeshCommands[Index];
        Resources.ForwardVisibleCommands.Append(WriteForwardMeshCommands);

        const TArray<uint32>& WriteDeferredMeshCommands = WriteableDeferredMeshCommands[Index];
        Resources.DeferredVisibleCommands.Append(WriteDeferredMeshCommands);
    }
}

void CRenderer::PerformFXAA(CRHICommandList& InCmdList)
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

    CRHIRenderTargetView* BackBufferRTV = Resources.BackBuffer->GetRenderTargetView();
    InCmdList.SetRenderTargets(&BackBufferRTV, 1, nullptr);

    CRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetDefaultShaderResourceView();
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

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End FXAA");
}

void CRenderer::PerformBackBufferBlit(CRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin Draw BackBuffer");

    TRACE_SCOPE("Draw to BackBuffer");

    CRHIRenderTargetView* BackBufferRTV = Resources.BackBuffer->GetRenderTargetView();
    InCmdList.SetRenderTargets(&BackBufferRTV, 1, nullptr);

    CRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetDefaultShaderResourceView();
    InCmdList.SetShaderResourceView(PostShader.Get(), FinalTargetSRV, 0);
    InCmdList.SetSamplerState(PostShader.Get(), Resources.GBufferSampler.Get(), 0);

    InCmdList.SetGraphicsPipelineState(PostPSO.Get());
    InCmdList.DrawInstanced(3, 1, 0, 0);

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End Draw BackBuffer");
}

void CRenderer::PerformAABBDebugPass(CRHICommandList& InCmdList)
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

        SAABB& Box = Command.Mesh->BoundingBox;

        CVector3 Scale    = CVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        CVector3 Position = Box.GetCenter();

        CMatrix4 TranslationMatrix = CMatrix4::Translation(Position.x, Position.y, Position.z);
        CMatrix4 ScaleMatrix       = CMatrix4::Scale(Scale.x, Scale.y, Scale.z);
        CMatrix4 TransformMatrix   = Command.CurrentActor->GetTransform().GetMatrix();
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

    // Init point light task
    const auto RenderPointShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderPointLightShadows(PointShadowCmdList, LightSetup, Scene);
    };

    if (!PointShadowTask.Delegate.IsBound())
    {
        PointShadowTask.Delegate.BindLambda(RenderPointShadows);
    }

    CAsyncTaskManager::Get().Dispatch(PointShadowTask);

    // Init directional light task
    const auto RenderDirShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderDirectionalLightShadows(DirShadowCmdList, LightSetup, Resources, Scene);
    };

    DirShadowTask.Delegate.BindLambda(RenderDirShadows);
    CAsyncTaskManager::Get().Dispatch(DirShadowTask);

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

    TStaticArray<float, 4> BlackClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearDepthStencilView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView(), 1.0f, 0);

    if (GPrePassEnabled.GetBool())
    {
        const auto RenderPrePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderPrePass(PrePassCmdList, Resources, Scene);
        };

        PrePassTask.Delegate.BindLambda(RenderPrePass);
        CAsyncTaskManager::Get().Dispatch(PrePassTask);
    }

    if (ShadingImage && GEnableVariableRateShading.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(ShadingRateCmdList, "Begin VRS Image");
        ShadingRateCmdList.SetShadingRate(EShadingRate::VRS_1x1);

        ShadingRateCmdList.TransitionTexture(ShadingImage.Get(), EResourceAccess::ShadingRateSource, EResourceAccess::UnorderedAccess);

        ShadingRateCmdList.SetComputePipelineState(ShadingRatePipeline.Get());

        CRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
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

    if (RHISupportsRayTracing())
    {
        const auto RenderRayTracing = [&]()
        {
            GPU_TRACE_SCOPE(RayTracingCmdList, "Ray Tracing");
            CRenderer::RayTracer.PreRender(RayTracingCmdList, Resources, Scene);
        };

        RayTracingTask.Delegate.BindLambda(RenderRayTracing);
        CAsyncTaskManager::Get().Dispatch(RayTracingTask);
    }

    {
        const auto RenderBasePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderBasePass(BasePassCmdList, Resources);
        };

        BasePassTask.Delegate.BindLambda(RenderBasePass);
        CAsyncTaskManager::Get().Dispatch(BasePassTask);
    }

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetDefaultShaderResourceView())
                   , Resources.GBuffer[GBUFFER_ALBEDO_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetDefaultShaderResourceView())
                   , Resources.GBuffer[GBUFFER_NORMAL_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetDefaultShaderResourceView())
                   , Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceAccess::RenderTarget, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetDefaultShaderResourceView())
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

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.SSAOBuffer->GetDefaultShaderResourceView())
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

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(LightSetup.DirectionalShadowMask->GetDefaultShaderResourceView())
                   , LightSetup.DirectionalShadowMask
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[0]->GetDefaultShaderResourceView())
                   , LightSetup.ShadowMapCascades[0]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[1]->GetDefaultShaderResourceView())
                   , LightSetup.ShadowMapCascades[1]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[2]->GetDefaultShaderResourceView())
                   , LightSetup.ShadowMapCascades[2]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[3]->GetDefaultShaderResourceView())
                   , LightSetup.ShadowMapCascades[3]
                   , EResourceAccess::NonPixelShaderResource
                   , EResourceAccess::NonPixelShaderResource);

    MainCmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    MainCmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    MainCmdList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.IntegrationLUT->GetDefaultShaderResourceView())
                   , Resources.IntegrationLUT
                   , EResourceAccess::PixelShaderResource
                   , EResourceAccess::PixelShaderResource);

    if (!Resources.ForwardVisibleCommands.IsEmpty())
    {
        GPU_TRACE_SCOPE(MainCmdList, "Forward Pass");
        ForwardRenderer.Render(MainCmdList, Resources, LightSetup);
    }

    MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.FinalTarget->GetDefaultShaderResourceView())
                   , Resources.FinalTarget
                   , EResourceAccess::PixelShaderResource
                   , EResourceAccess::PixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource);

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDefaultShaderResourceView())
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

        if (RHISupportsVariableRateShading())
        {
            MainCmdList.SetShadingRate(EShadingRate::VRS_1x1);
            MainCmdList.SetShadingRateImage(nullptr);
        }

        CApplicationInstance::Get().DrawWindows(MainCmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "End UI Render");

    MainCmdList.TransitionTexture(Resources.BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "--END FRAME--");

    CGPUProfiler::Get().EndGPUFrame(MainCmdList);

#if 1
    MainCmdList.EndExternalCapture();
#endif

    CAsyncTaskManager::Get().WaitForAll();

    {
        TRACE_SCOPE("ExecuteCommandList");

        CRHICommandList* CmdLists[9] =
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

        CRHICommandQueue::Get().ExecuteCommandLists(CmdLists, ArrayCount(CmdLists));

        FrameStatistics.NumDrawCalls      = CRHICommandQueue::Get().GetNumDrawCalls();
        FrameStatistics.NumDispatchCalls  = CRHICommandQueue::Get().GetNumDispatchCalls();
        FrameStatistics.NumRenderCommands = CRHICommandQueue::Get().GetNumCommands();
    }

    {
        TRACE_SCOPE("Present");
        Resources.MainWindowViewport->Present(GVSyncEnabled.GetBool());
    }
}

void CRenderer::Release()
{
    CRHICommandQueue::Get().WaitForGPU();

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

    if (CApplicationInstance::IsInitialized())
    {
        CApplicationInstance& Application = CApplicationInstance::Get();
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
        CDebug::DebugBreak();
        return;
    }

    if (!DeferredRenderer.ResizeResources(Resources))
    {
        CDebug::DebugBreak();
        return;
    }

    if (!SSAORenderer.ResizeResources(Resources))
    {
        CDebug::DebugBreak();
        return;
    }

    if (!ShadowMapRenderer.ResizeResources(Width, Height, LightSetup))
    {
        CDebug::DebugBreak();
        return;
    }
}

bool CRenderer::InitBoundingBoxDebugPass()
{
    TArray<uint8> ShaderCode;
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Debug.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    AABBVertexShader = RHICreateVertexShader(ShaderCode);
    if (!AABBVertexShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Debug.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    AABBPixelShader = RHICreatePixelShader(ShaderCode);
    if (!AABBPixelShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIVertexInputLayoutInitializer InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, 0, 0, EVertexInputClass::Vertex, 0 },
    };

    TSharedRef<CRHIVertexInputLayout> InputLayoutState = RHICreateVertexInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIDepthStencilStateInitializer DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIBlendStateInitializer BlendStateInfo;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIGraphicsPipelineStateInitializer PSOProperties;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.VertexInputLayout                      = InputLayoutState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = AABBVertexShader.Get();
    PSOProperties.ShaderState.PixelShader                = AABBPixelShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Line;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = Resources.RenderTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = Resources.DepthBufferFormat;

    AABBDebugPipelineState = RHICreateGraphicsPipelineState(PSOProperties);
    if (!AABBDebugPipelineState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBDebugPipelineState->SetName("Debug PipelineState");
    }

    TStaticArray<CVector3, 8> Vertices =
    {
        CVector3(-0.5f, -0.5f,  0.5f),
        CVector3( 0.5f, -0.5f,  0.5f),
        CVector3(-0.5f,  0.5f,  0.5f),
        CVector3( 0.5f,  0.5f,  0.5f),

        CVector3( 0.5f, -0.5f, -0.5f),
        CVector3(-0.5f, -0.5f, -0.5f),
        CVector3( 0.5f,  0.5f, -0.5f),
        CVector3(-0.5f,  0.5f, -0.5f)
    };

    CRHIBufferDataInitializer VertexData(Vertices.Data(), Vertices.SizeInBytes());

    CRHIVertexBufferInitializer VBInitializer(EBufferUsageFlags::Default, Vertices.Size(), sizeof(CVector3), EResourceAccess::Common, &VertexData);
    AABBVertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!AABBVertexBuffer)
    {
        CDebug::DebugBreak();
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

    CRHIBufferDataInitializer IndexData(Indices.Data(), Indices.SizeInBytes());

    CRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, EIndexFormat::uint16, Indices.Size(), EResourceAccess::Common, &IndexData);
    AABBIndexBuffer = RHICreateIndexBuffer(IBInitializer);
    if (!AABBIndexBuffer)
    {
        CDebug::DebugBreak();
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
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/FullscreenVS.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CRHIVertexShader> VShader = RHICreateVertexShader(ShaderCode);
    if (!VShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/PostProcessPS.hlsl", "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    PostShader = RHICreatePixelShader(ShaderCode);
    if (!PostShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIDepthStencilStateInitializer DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::Always;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIBlendStateInitializer BlendStateInfo;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIGraphicsPipelineStateInitializer PSOProperties;
    PSOProperties.VertexInputLayout                      = nullptr;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PostShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    PostPSO = RHICreateGraphicsPipelineState(PSOProperties);
    if (!PostPSO)
    {
        CDebug::DebugBreak();
        return false;
    }

    // FXAA
    CRHISamplerStateInitializer CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    Resources.FXAASampler = RHICreateSamplerState(CreateInfo);
    if (!Resources.FXAASampler)
    {
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/FXAA_PS.hlsl", "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    FXAAShader = RHICreatePixelShader(ShaderCode);
    if (!FXAAShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    PSOProperties.ShaderState.PixelShader = FXAAShader.Get();

    FXAAPSO = RHICreateGraphicsPipelineState(PSOProperties);
    if (!FXAAPSO)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FXAAPSO->SetName("FXAA PipelineState");
    }

    TArray<SShaderDefine> Defines =
    {
        SShaderDefine("ENABLE_DEBUG", "1")
    };

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/FXAA_PS.hlsl", "Main", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    FXAADebugShader = RHICreatePixelShader(ShaderCode);
    if (!FXAADebugShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    PSOProperties.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = RHICreateGraphicsPipelineState(PSOProperties);
    if (!FXAADebugPSO)
    {
        CDebug::DebugBreak();
        return false;
    }

    return true;
}

bool CRenderer::InitShadingImage()
{
    SShadingRateSupport Support;
    RHIQueryShadingRateSupport(Support);

    if (Support.Tier != ERHIShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0)
    {
        return true;
    }

    const uint32 Width  = Resources.MainWindowViewport->GetWidth() / Support.ShadingRateImageTileSize;
    const uint32 Height = Resources.MainWindowViewport->GetHeight() / Support.ShadingRateImageTileSize;

    CRHITexture2DInitializer Initializer(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::RWTexture, EResourceAccess::ShadingRateSource);
    ShadingImage = RHICreateTexture2D(Initializer);
    if (!ShadingImage)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        ShadingImage->SetName("Shading Rate Image");
    }

    TArray<uint8> ShaderCode;
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ShadingImage.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    ShadingRateShader = RHICreateComputeShader(ShaderCode);
    if (!ShadingRateShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIComputePipelineStateInitializer PSOInitializer(ShadingRateShader.Get());
    ShadingRatePipeline = RHICreateComputePipelineState(PSOInitializer);
    if (!ShadingRatePipeline)
    {
        CDebug::DebugBreak();
        return false;
    }

    return true;
}
