#include "Renderer.h"

#include "Application/ApplicationInterface.h"

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
#include "Core/Platform/PlatformThreadMisc.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variables

TAutoConsoleVariable<bool> GEnableSSAO("Renderer.Feature.SSAO", true);

TAutoConsoleVariable<bool> GEnableFXAA("Renderer.Feature.FXAA", true);
TAutoConsoleVariable<bool> GFXAADebug("Renderer.Debug.FXAADebug", false);

TAutoConsoleVariable<bool> GEnableVariableRateShading("Renderer.Feature.VariableRateShading", false);

TAutoConsoleVariable<bool> GPrePassEnabled("Renderer.Feature.PrePass", true);
TAutoConsoleVariable<bool> GDrawAABBs("Renderer.Debug.DrawAABBs", false);
TAutoConsoleVariable<bool> GVSyncEnabled("Renderer.Feature.VerticalSync", false);
TAutoConsoleVariable<bool> GFrustumCullEnabled("Renderer.Feature.FrustumCulling", true);
TAutoConsoleVariable<bool> GRayTracingEnabled("Renderer.Feature.RayTracing", false);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCameraBuffer

struct FCameraBuffer
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

RENDERER_API FRenderer GRenderer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRenderer

FRenderer::FRenderer()
    : WindowHandler(MakeShared<FRendererWindowHandler>())
{ }

bool FRenderer::Init()
{
    FRHIViewportInitializer ViewportInitializer(
        GEngine->MainWindow->GetPlatformHandle(),
        EFormat::R8G8B8A8_Unorm,
        EFormat::Unknown,
        GEngine->MainWindow->GetWidth(),
        GEngine->MainWindow->GetHeight());

    Resources.MainWindowViewport = RHICreateViewport(ViewportInitializer);
    if (!Resources.MainWindowViewport)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        // Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    FRHIConstantBufferInitializer CBInitializer(EBufferUsageFlags::Default, sizeof(FCameraBuffer), EResourceAccess::Common);
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
        FRHISamplerStateInitializer Initializer;
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
        FRHISamplerStateInitializer Initializer;
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

    LightProbeRenderer.RenderSkyLightProbe(CommandList, LightSetup, Resources);
    GRHICommandExecutor.ExecuteCommandList(CommandList);

    FApplicationInterface& Application = FApplicationInterface::Get();

    // Register EventFunc
    WindowHandler->WindowResizedDelegate.BindRaw(this, &FRenderer::OnWindowResize);
    Application.AddWindowMessageHandler(WindowHandler, uint32(-1));

    // Register Windows
    TextureDebugger = FRenderTargetDebugWindow::Create();
    Application.AddWindow(TextureDebugger);

    InfoWindow = FRendererInfoWindow::Create();
    Application.AddWindow(InfoWindow);

    GPUProfilerWindow = FGPUProfilerWindow::Create();
    Application.AddWindow(GPUProfilerWindow);

    return true;
}

void FRenderer::FrustumCullingAndSortingInternal(
    const FCamera* Camera,
    const TPair<uint32, uint32>& DrawCommands,
    TArray<uint32>& OutDeferredDrawCommands,
    TArray<uint32>& OutForwardDrawCommands)
{

    TRACE_SCOPE("Frustum Culling And Sorting Inner");

    // Inserts a mesh based on distance
    const auto InsertSorted = [](
        int32 CommandIndex,
        const FCamera* Camera,
        const FVector3& WorldPosition,
        TArray<float>& OutDistances,
        TArray<uint32>& OutCommands) -> void
    {
        Check(OutDistances.GetSize() == OutCommands.GetSize());

        FVector3 CameraPosition = Camera->GetPosition();
        FVector3 DistanceVector = WorldPosition - CameraPosition;

        const float NewDistance = DistanceVector.LengthSquared();

        int32 Index = 0;
        for (; Index < OutCommands.GetSize(); ++Index)
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

        const FMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];

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
    const auto NumMeshCommands      = Scene.GetMeshDrawCommands().GetSize();
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
            FrustumCullingAndSortingInternal(
                CameraPtr, 
                ReadMeshCommands, 
                WriteDeferredMeshCommands, 
                WriteForwardMeshCommands);
        };

        FAsyncTask AsyncTask;
        AsyncTask.Delegate.BindLambda(CullAndSort);

        Tasks[Index] = FTaskManagerInterface::Get().Dispatch(AsyncTask);
    }

    // Sync and insert
    for (uint32 Index = 0; Index < NumThreads; ++Index)
    {
        FTaskManagerInterface::Get().WaitFor(Tasks[Index], true);
        
        const TArray<uint32>& WriteForwardMeshCommands = WriteableForwardMeshCommands[Index];
        Resources.ForwardVisibleCommands.Append(WriteForwardMeshCommands);

        const TArray<uint32>& WriteDeferredMeshCommands = WriteableDeferredMeshCommands[Index];
        Resources.DeferredVisibleCommands.Append(WriteDeferredMeshCommands);
    }
}

void FRenderer::PerformFXAA(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(InCmdList, "FXAA");

    struct FFXAASettings
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

void FRenderer::PerformBackBufferBlit(FRHICommandList& InCmdList)
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

void FRenderer::PerformAABBDebugPass(FRHICommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin DebugPass");

    TRACE_SCOPE("DebugPass");

    InCmdList.SetGraphicsPipelineState(AABBDebugPipelineState.Get());

    InCmdList.SetPrimitiveTopology(EPrimitiveTopology::LineList);

    InCmdList.SetConstantBuffer(AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0);

    InCmdList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
    InCmdList.SetIndexBuffer(AABBIndexBuffer.Get());

    for (const auto CommandIndex : Resources.DeferredVisibleCommands)
    {
        const FMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[CommandIndex];

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

void FRenderer::Tick(const FScene& Scene)
{
    Resources.BackBuffer             = Resources.MainWindowViewport->GetBackBuffer();
    Resources.GlobalMeshDrawCommands = TArrayView<const FMeshDrawCommand>(Scene.GetMeshDrawCommands());

    // Prepare Lights
#if 1
    CommandList.BeginExternalCapture();
#endif

    FGPUProfiler::Get().BeginGPUFrame(CommandList);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--BEGIN FRAME--");

    LightSetup.BeginFrame(CommandList, Scene);

    // Initialize point light task
    ShadowMapRenderer.RenderPointLightShadows(CommandList, LightSetup, Scene);

    // Initialize directional light task
    ShadowMapRenderer.RenderDirectionalLightShadows(CommandList, LightSetup, Resources, Scene);

    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();

    // Clear the images that were debug gable last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    // FrustumCulling
    if (!GFrustumCullEnabled.GetBool())
    {
        for (int32 CommandIndex = 0; CommandIndex < Resources.GlobalMeshDrawCommands.GetSize(); ++CommandIndex)
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
    FCameraBuffer CamBuffer;
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

    CommandList.TransitionBuffer(
        Resources.CameraBuffer.Get(), 
        EResourceAccess::VertexAndConstantBuffer, 
        EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(
        Resources.CameraBuffer.Get(), 
        0, 
        sizeof(FCameraBuffer), 
        &CamBuffer);
    CommandList.TransitionBuffer(
        Resources.CameraBuffer.Get(), 
        EResourceAccess::CopyDest, 
        EResourceAccess::VertexAndConstantBuffer);

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::RenderTarget);
    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
        EResourceAccess::PixelShaderResource, 
        EResourceAccess::DepthWrite);

    // PrePass
    if (GPrePassEnabled.GetBool())
    {
        DeferredRenderer.RenderPrePass(CommandList, Resources, Scene);
    }

#if 0
    if (ShadingImage && GEnableVariableRateShading.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin VRS Image");
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);

        CommandList.TransitionTexture(
            ShadingImage.Get(), 
            EResourceAccess::ShadingRateSource, 
            EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(ShadingRatePipeline.Get());

        FRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        CommandList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

        CommandList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);

        CommandList.TransitionTexture(
            ShadingImage.Get(), 
            EResourceAccess::UnorderedAccess, 
            EResourceAccess::ShadingRateSource);

        CommandList.SetShadingRateImage(ShadingImage.Get());

        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End VRS Image");
    }
    else if (RHISupportsVariableRateShading())
    {
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);
    }
#endif

    // RayTracing PrePass
    if (RHISupportsRayTracing())
    {
        GPU_TRACE_SCOPE(CommandList, "Ray Tracing");
        RayTracer.PreRender(CommandList, Resources, Scene);
    }

    // BasePass
    {
        DeferredRenderer.RenderBasePass(CommandList, Resources);
    }

    // Start recording the main CommandList
    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
        EResourceAccess::RenderTarget, 
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
        EResourceAccess::RenderTarget, 
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_NORMAL_INDEX],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        EResourceAccess::RenderTarget, 
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
        EResourceAccess::RenderTarget, 
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
        EResourceAccess::DepthWrite, 
        EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(
        Resources.SSAOBuffer.Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::UnorderedAccess);

    CommandList.ClearUnorderedAccessView(Resources.SSAOBuffer->GetUnorderedAccessView(), FVector4{ 1.0f, 1.0f, 1.0f, 1.0f });

    if (GEnableSSAO.GetBool())
    {
        GPU_TRACE_SCOPE(CommandList, "SSAO");
        SSAORenderer.Render(CommandList, Resources);
    }

    CommandList.TransitionTexture(
        Resources.SSAOBuffer.Get(), 
        EResourceAccess::UnorderedAccess, 
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.SSAOBuffer->GetShaderResourceView()),
        Resources.SSAOBuffer,
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    {
        CommandList.TransitionTexture(
            Resources.FinalTarget.Get(), 
            EResourceAccess::PixelShaderResource, 
            EResourceAccess::UnorderedAccess);
        CommandList.TransitionTexture(
            Resources.BackBuffer, 
            EResourceAccess::Present, 
            EResourceAccess::RenderTarget);
        CommandList.TransitionTexture(
            LightSetup.IrradianceMap.Get(), 
            EResourceAccess::PixelShaderResource, 
            EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(
            LightSetup.SpecularIrradianceMap.Get(), 
            EResourceAccess::PixelShaderResource, 
            EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(
            Resources.IntegrationLUT.Get(), 
            EResourceAccess::PixelShaderResource, 
            EResourceAccess::NonPixelShaderResource);

        ShadowMapRenderer.RenderShadowMasks(CommandList, LightSetup, Resources);

        DeferredRenderer.RenderDeferredTiledLightPass(CommandList, Resources, LightSetup);
    }

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(
        Resources.FinalTarget.Get(), 
        EResourceAccess::UnorderedAccess, 
        EResourceAccess::RenderTarget);

    SkyboxRenderPass.Render(CommandList, Resources, Scene);

    CommandList.TransitionTexture(
        LightSetup.PointLightShadowMaps.Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::PixelShaderResource);

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

    CommandList.TransitionTexture(
        LightSetup.IrradianceMap.Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(
        LightSetup.SpecularIrradianceMap.Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(
        Resources.IntegrationLUT.Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::PixelShaderResource);

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

    CommandList.TransitionTexture(
        Resources.FinalTarget.Get(), 
        EResourceAccess::RenderTarget, 
        EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.FinalTarget->GetShaderResourceView()),
        Resources.FinalTarget,
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    CommandList.TransitionTexture(
        Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
        EResourceAccess::DepthWrite, 
        EResourceAccess::PixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_DEPTH_INDEX],
        EResourceAccess::PixelShaderResource,
        EResourceAccess::PixelShaderResource);

    if (GEnableFXAA.GetBool())
    {
        PerformFXAA(CommandList);
    }
    else
    {
        PerformBackBufferBlit(CommandList);
    }

    if (GDrawAABBs.GetBool())
    {
        PerformAABBDebugPass(CommandList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin UI Render");

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

        CommandList.BeginRenderPass(RenderPass);
        
        FApplicationInterface::Get().DrawWindows(CommandList);
        
        CommandList.EndRenderPass();
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End UI Render");

    CommandList.TransitionTexture(
        Resources.BackBuffer, 
        EResourceAccess::RenderTarget, 
        EResourceAccess::Present);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "--END FRAME--");

    FGPUProfiler::Get().EndGPUFrame(CommandList);

#if 1
    CommandList.EndExternalCapture();
#endif

    CommandList.PresentViewport(Resources.MainWindowViewport.Get(), GVSyncEnabled.GetBool());

    {
        TRACE_SCOPE("ExecuteCommandList");

        GRHICommandExecutor.WaitForOutstandingTasks();
        GRHICommandExecutor.ExecuteCommandList(CommandList);
        FrameStatistics = GRHICommandExecutor.GetStatistics();
    }
}

void FRenderer::Release()
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

    if (FApplicationInterface::IsInitialized())
    {
        FApplicationInterface& Application = FApplicationInterface::Get();
        Application.RemoveWindow(TextureDebugger);
        TextureDebugger.Reset();

        Application.RemoveWindow(InfoWindow);
        InfoWindow.Reset();

        Application.RemoveWindow(GPUProfilerWindow);
        GPUProfilerWindow.Reset();
    }
}

void FRenderer::OnWindowResize(const FWindowResizeEvent& Event)
{
    const uint32 Width  = Event.Width;
    const uint32 Height = Event.Height;

    GRHICommandExecutor.WaitForOutstandingTasks();

    if (!Resources.MainWindowViewport->Resize(Width, Height))
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
}

bool FRenderer::InitBoundingBoxDebugPass()
{
    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    AABBVertexShader = RHICreateVertexShader(ShaderCode);
    if (!AABBVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    AABBPixelShader = RHICreatePixelShader(ShaderCode);
    if (!AABBPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIVertexInputLayoutInitializer InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 },
    };

    FRHIVertexInputLayoutRef InputLayoutState = RHICreateVertexInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilInitializer.bDepthEnable   = false;
    DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::Zero;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInfo;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        DEBUG_BREAK();
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
        DEBUG_BREAK();
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

    FRHIBufferDataInitializer VertexData(Vertices.GetData(), Vertices.SizeInBytes());

    FRHIVertexBufferInitializer VBInitializer(EBufferUsageFlags::Default, Vertices.GetSize(), sizeof(FVector3), EResourceAccess::Common, &VertexData);
    AABBVertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!AABBVertexBuffer)
    {
        DEBUG_BREAK();
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

    FRHIBufferDataInitializer IndexData(Indices.GetData(), Indices.SizeInBytes());

    FRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, EIndexFormat::uint16, Indices.GetSize(), EResourceAccess::Common, &IndexData);
    AABBIndexBuffer = RHICreateIndexBuffer(IBInitializer);
    if (!AABBIndexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBIndexBuffer->SetName("AABB IndexBuffer");
    }

    return true;
}

bool FRenderer::InitAA()
{
    TArray<uint8> ShaderCode;
    
    {
		FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Vertex);
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
		FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel);
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
    DepthStencilInitializer.DepthFunc      = EComparisonFunc::Always;
    DepthStencilInitializer.bDepthEnable   = false;
    DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::Zero;

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

    FRHIBlendStateInitializer BlendStateInfo;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInfo);
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
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel, Defines.CreateView());
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
        DEBUG_BREAK();
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
