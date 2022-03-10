#include "Renderer.h"

#include "Application/ApplicationInstance.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/TextureFactory.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Engine.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Frustum.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variables

TAutoConsoleVariable<bool> GEnableSSAO("renderer.EnableSSAO", true);

TAutoConsoleVariable<bool> GEnableFXAA("renderer.EnableFXAA", true);
TAutoConsoleVariable<bool> GFXAADebug("renderer.FXAADebug", false);

TAutoConsoleVariable<bool> GEnableVariableRateShading("renderer.EnableVariableRateShading", false);

TAutoConsoleVariable<bool> GPrePassEnabled("renderer.EnablePrePass", true);
TAutoConsoleVariable<bool> GDrawAABBs("renderer.EnableDrawAABBs", false);
TAutoConsoleVariable<bool> GVSyncEnabled("renderer.EnableVerticalSync", false);
TAutoConsoleVariable<bool> GFrustumCullEnabled("renderer.EnableFrustumCulling", true);
TAutoConsoleVariable<bool> GRayTracingEnabled("renderer.EnableRayTracing", true);

//static const uint32 ShadowMapSampleCount = 2;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CameraBufferDesc

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRenderer

RENDERER_API CRenderer GRenderer;

CRenderer::CRenderer()
    : WindowHandler(MakeShared<CRendererWindowHandler>())
{
}

bool CRenderer::Init()
{
    Resources.MainWindowViewport = RHICreateViewport(GEngine->MainWindow->GetPlatformHandle(), 0, 0, Resources.BackBufferFormat, ERHIFormat::Unknown);
    if (!Resources.MainWindowViewport)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    Resources.CameraBuffer = RHICreateConstantBuffer<SCameraBufferDesc>(BufferFlag_Default, ERHIResourceState::Common, nullptr);
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
    SRHIInputLayoutStateDesc InputLayout =
    {
        { "POSITION", 0, ERHIFormat::R32G32B32_Float, 0, 0,  EInputClassification::Vertex, 0 },
        { "NORMAL",   0, ERHIFormat::R32G32B32_Float, 0, 12, EInputClassification::Vertex, 0 },
        { "TANGENT",  0, ERHIFormat::R32G32B32_Float, 0, 24, EInputClassification::Vertex, 0 },
        { "TEXCOORD", 0, ERHIFormat::R32G32_Float,    0, 36, EInputClassification::Vertex, 0 },
    };

    Resources.StdInputLayout = RHICreateInputLayout(InputLayout);
    if (!Resources.StdInputLayout)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        Resources.StdInputLayout->SetName("Standard InputLayoutState");
    }

    {
        CRHISamplerStateDesc CreateInfo;
        CreateInfo.AddressU    = ERHISamplerMode::Border;
        CreateInfo.AddressV    = ERHISamplerMode::Border;
        CreateInfo.AddressW    = ERHISamplerMode::Border;
        CreateInfo.Filter      = ERHISamplerFilter::MinMagMipPoint;
        CreateInfo.BorderColor = SColorF(1.0f, 1.0f, 1.0f, 1.0f);

        Resources.DirectionalLightShadowSampler = RHICreateSamplerState(CreateInfo);
        if (!Resources.DirectionalLightShadowSampler)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            Resources.DirectionalLightShadowSampler->SetName("ShadowMap Sampler");
        }
    }

    {
        CRHISamplerStateDesc CreateInfo;
        CreateInfo.AddressU       = ERHISamplerMode::Wrap;
        CreateInfo.AddressV       = ERHISamplerMode::Wrap;
        CreateInfo.AddressW       = ERHISamplerMode::Wrap;
        CreateInfo.Filter         = ERHISamplerFilter::Comparison_MinMagMipLinear;
        CreateInfo.ComparisonFunc = ERHIComparisonFunc::LessEqual;

        Resources.PointLightShadowSampler = RHICreateSamplerState(CreateInfo);
        if (!Resources.PointLightShadowSampler)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            Resources.PointLightShadowSampler->SetName("ShadowMap Comparison Sampler");
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

void CRenderer::PerformFrustumCulling(const CScene& Scene)
{
    TRACE_SCOPE("Frustum Culling");

    CCamera* Camera = Scene.GetCamera();
    CFrustum CameraFrustum = CFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
    {
        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        CVector3 Top = CVector3(&Command.Mesh->BoundingBox.Top.x);
        Top = TransformMatrix.TransformPosition(Top);
        CVector3 Bottom = CVector3(&Command.Mesh->BoundingBox.Bottom.x);
        Bottom = TransformMatrix.TransformPosition(Bottom);

        SAABB Box;
        Box.Top = Top;
        Box.Bottom = Bottom;
        if (CameraFrustum.CheckAABB(Box))
        {
            if (Command.Material->ShouldRenderInForwardPass())
            {
                Resources.ForwardVisibleCommands.Emplace(Command);
            }
            else
            {
                Resources.DeferredVisibleCommands.Emplace(Command);
            }
        }
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

    CRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
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

    CRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
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

    InCmdList.SetPrimitiveTopology(ERHIPrimitiveTopology::LineList);

    InCmdList.SetConstantBuffer(AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0);

    InCmdList.SetVertexBuffers(&AABBVertexBuffer, 1, 0);
    InCmdList.SetIndexBuffer(AABBIndexBuffer.Get());

    for (const SMeshDrawCommand& Command : Resources.DeferredVisibleCommands)
    {
        SAABB& Box = Command.Mesh->BoundingBox;
        CVector3 Scale = CVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        CVector3 Position = Box.GetCenter();

        CMatrix4 TranslationMatrix = CMatrix4::Translation(Position.x, Position.y, Position.z);
        CMatrix4 ScaleMatrix = CMatrix4::Scale(Scale.x, Scale.y, Scale.z);
        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
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
    Resources.BackBuffer = Resources.MainWindowViewport->GetBackBuffer();

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

    CDispatchQueue::Get().Dispatch(PointShadowTask);

    // Init directional light task
    const auto RenderDirShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderDirectionalLightShadows(DirShadowCmdList, LightSetup, Resources, Scene);
    };

    DirShadowTask.Delegate.BindLambda(RenderDirShadows);
    CDispatchQueue::Get().Dispatch(DirShadowTask);

    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();

    // Clear the images that were debuggable last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    if (!GFrustumCullEnabled.GetBool())
    {
        for (const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
        {
            if (Command.Material->HasAlphaMask())
            {
                Resources.ForwardVisibleCommands.Emplace(Command);
            }
            else
            {
                Resources.DeferredVisibleCommands.Emplace(Command);
            }
        }
    }
    else
    {
        PerformFrustumCulling(Scene);
    }

    // Update camera-buffer
    SCameraBufferDesc CamBuff;
    CamBuff.ViewProjection    = Scene.GetCamera()->GetViewProjectionMatrix();
    CamBuff.View              = Scene.GetCamera()->GetViewMatrix();
    CamBuff.ViewInv           = Scene.GetCamera()->GetViewInverseMatrix();
    CamBuff.Projection        = Scene.GetCamera()->GetProjectionMatrix();
    CamBuff.ProjectionInv     = Scene.GetCamera()->GetProjectionInverseMatrix();
    CamBuff.ViewProjectionInv = Scene.GetCamera()->GetViewProjectionInverseMatrix();
    CamBuff.Position          = Scene.GetCamera()->GetPosition();
    CamBuff.Forward           = Scene.GetCamera()->GetForward();
    CamBuff.Right             = Scene.GetCamera()->GetRight();
    CamBuff.NearPlane         = Scene.GetCamera()->GetNearPlane();
    CamBuff.FarPlane          = Scene.GetCamera()->GetFarPlane();
    CamBuff.AspectRatio       = Scene.GetCamera()->GetAspectRatio();

    PrepareGBufferCmdList.TransitionBuffer(Resources.CameraBuffer.Get(), ERHIResourceState::VertexAndConstantBuffer, ERHIResourceState::CopyDest);

    PrepareGBufferCmdList.UpdateBuffer(Resources.CameraBuffer.Get(), 0, sizeof(SCameraBufferDesc), &CamBuff);

    PrepareGBufferCmdList.TransitionBuffer(Resources.CameraBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::VertexAndConstantBuffer);

    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::RenderTarget);
    PrepareGBufferCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::DepthWrite);

    SColorF BlackClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    PrepareGBufferCmdList.ClearDepthStencilView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView(), SDepthStencil(1.0f, 0));

    if (GPrePassEnabled.GetBool())
    {
        const auto RenderPrePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderPrePass(PrePassCmdList, Resources, Scene);
        };

        PrePassTask.Delegate.BindLambda(RenderPrePass);
        CDispatchQueue::Get().Dispatch(PrePassTask);
    }

    if (ShadingImage && GEnableVariableRateShading.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(ShadingRateCmdList, "Begin VRS Image");
        ShadingRateCmdList.SetShadingRate(ERHIShadingRate::VRS_1x1);

        ShadingRateCmdList.TransitionTexture(ShadingImage.Get(), ERHIResourceState::ShadingRateSource, ERHIResourceState::UnorderedAccess);

        ShadingRateCmdList.SetComputePipelineState(ShadingRatePipeline.Get());

        CRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        ShadingRateCmdList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);

        ShadingRateCmdList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);

        ShadingRateCmdList.TransitionTexture(ShadingImage.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::ShadingRateSource);

        ShadingRateCmdList.SetShadingRateImage(ShadingImage.Get());

        INSERT_DEBUG_CMDLIST_MARKER(ShadingRateCmdList, "End VRS Image");
    }
    else if (RHISupportsVariableRateShading())
    {
        ShadingRateCmdList.SetShadingRate(ERHIShadingRate::VRS_1x1);
    }

    if ( /* DISABLES CODE */ (false) /*IsRayTracingSupported())*/)
    {
        const auto RenderRayTracing = [&]()
        {
            GPU_TRACE_SCOPE(RayTracingCmdList, "Ray Tracing");
            CRenderer::RayTracer.PreRender(RayTracingCmdList, Resources, Scene);
        };

        RayTracingTask.Delegate.BindLambda(RenderRayTracing);
        CDispatchQueue::Get().Dispatch(RayTracingTask);
    }

    {
        const auto RenderBasePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderBasePass(BasePassCmdList, Resources);
        };

        BasePassTask.Delegate.BindLambda(RenderBasePass);
        CDispatchQueue::Get().Dispatch(BasePassTask);
    }

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_NORMAL_INDEX],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), ERHIResourceState::RenderTarget, ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), ERHIResourceState::DepthWrite, ERHIResourceState::NonPixelShaderResource);
    MainCmdList.TransitionTexture(Resources.SSAOBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess);

    const SColorF WhiteColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    MainCmdList.ClearUnorderedAccessView(Resources.SSAOBuffer->GetUnorderedAccessView(), WhiteColor);

    if (GEnableSSAO.GetBool())
    {
        GPU_TRACE_SCOPE(MainCmdList, "SSAO");
        SSAORenderer.Render(MainCmdList, Resources);
    }

    MainCmdList.TransitionTexture(Resources.SSAOBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.SSAOBuffer->GetShaderResourceView()),
        Resources.SSAOBuffer,
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    {
        MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::UnorderedAccess);
        MainCmdList.TransitionTexture(Resources.BackBuffer, ERHIResourceState::Present, ERHIResourceState::RenderTarget);
        MainCmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource);
        MainCmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource);
        MainCmdList.TransitionTexture(Resources.IntegrationLUT.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource);

        ShadowMapRenderer.RenderShadowMasks(MainCmdList, LightSetup, Resources);

        DeferredRenderer.RenderDeferredTiledLightPass(MainCmdList, Resources, LightSetup);
    }

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::DepthWrite);
    MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::RenderTarget);

    SkyboxRenderPass.Render(MainCmdList, Resources, Scene);

    MainCmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(LightSetup.DirectionalShadowMask->GetShaderResourceView()),
        LightSetup.DirectionalShadowMask,
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[0]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[0],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[1]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[1],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[2]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[2],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(LightSetup.ShadowMapCascades[3]->GetShaderResourceView()),
        LightSetup.ShadowMapCascades[3],
        ERHIResourceState::NonPixelShaderResource,
        ERHIResourceState::NonPixelShaderResource);

    MainCmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource);
    MainCmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource);
    MainCmdList.TransitionTexture(Resources.IntegrationLUT.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.IntegrationLUT->GetShaderResourceView()),
        Resources.IntegrationLUT,
        ERHIResourceState::PixelShaderResource,
        ERHIResourceState::PixelShaderResource);

    if (!Resources.ForwardVisibleCommands.IsEmpty())
    {
        GPU_TRACE_SCOPE(MainCmdList, "Forward Pass");
        ForwardRenderer.Render(MainCmdList, Resources, LightSetup);
    }

    MainCmdList.TransitionTexture(Resources.FinalTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::PixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.FinalTarget->GetShaderResourceView()),
        Resources.FinalTarget,
        ERHIResourceState::PixelShaderResource,
        ERHIResourceState::PixelShaderResource);

    MainCmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), ERHIResourceState::DepthWrite, ERHIResourceState::PixelShaderResource);

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_DEPTH_INDEX],
        ERHIResourceState::PixelShaderResource,
        ERHIResourceState::PixelShaderResource);

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
            MainCmdList.SetShadingRate(ERHIShadingRate::VRS_1x1);
            MainCmdList.SetShadingRateImage(nullptr);
        }

        CApplicationInstance::Get().DrawWindows(MainCmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "End UI Render");
    
    MainCmdList.TransitionTexture(Resources.BackBuffer, ERHIResourceState::RenderTarget, ERHIResourceState::Present);

    INSERT_DEBUG_CMDLIST_MARKER(MainCmdList, "--END FRAME--");

    CGPUProfiler::Get().EndGPUFrame(MainCmdList);

#if 1
    MainCmdList.EndExternalCapture();
#endif

    CDispatchQueue::Get().WaitForAll();

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
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Debug.hlsl", "VSMain", nullptr, ERHIShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        AABBVertexShader->SetName("Debug VertexShader");
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Debug.hlsl", "PSMain", nullptr, ERHIShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        AABBPixelShader->SetName("Debug PixelShader");
    }

    SRHIInputLayoutStateDesc InputLayout =
    {
        { "POSITION", 0, ERHIFormat::R32G32B32_Float, 0, 0, EInputClassification::Vertex, 0 },
    };

    TSharedRef<CRHIInputLayoutState> InputLayoutState = RHICreateInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        InputLayoutState->SetName("Debug InputLayoutState");
    }

    SRHIDepthStencilStateDesc DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = ERHIComparisonFunc::LessEqual;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("Debug DepthStencilState");
    }

    SRHIRasterizerStateDesc RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Debug RasterizerState");
    }

    SRHIBlendStateDesc BlendStateInfo;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Debug BlendState");
    }

    SRHIGraphicsPipelineStateDesc PSOProperties;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.InputLayoutState                       = InputLayoutState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = AABBVertexShader.Get();
    PSOProperties.ShaderState.PixelShader                = AABBPixelShader.Get();
    PSOProperties.PrimitiveTopologyType                  = ERHIPrimitiveTopologyType::Line;
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
        CVector3(0.5f, -0.5f,  0.5f),
        CVector3(-0.5f,  0.5f,  0.5f),
        CVector3(0.5f,  0.5f,  0.5f),

        CVector3(0.5f, -0.5f, -0.5f),
        CVector3(-0.5f, -0.5f, -0.5f),
        CVector3(0.5f,  0.5f, -0.5f),
        CVector3(-0.5f,  0.5f, -0.5f)
    };

    SRHIResourceData VertexData(Vertices.Data(), Vertices.SizeInBytes());

    AABBVertexBuffer = RHICreateVertexBuffer<CVector3>(Vertices.Size(), BufferFlag_Default, ERHIResourceState::Common, &VertexData);
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

    SRHIResourceData IndexData(Indices.Data(), Indices.SizeInBytes());

    AABBIndexBuffer = RHICreateIndexBuffer(ERHIIndexFormat::uint16, Indices.Size(), BufferFlag_Default, ERHIResourceState::Common, &IndexData);
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
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/FullscreenVS.hlsl", "Main", nullptr, ERHIShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        VShader->SetName("Fullscreen VertexShader");
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/PostProcessPS.hlsl", "Main", nullptr, ERHIShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        PostShader->SetName("PostProcess PixelShader");
    }

    SRHIDepthStencilStateDesc DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = ERHIComparisonFunc::Always;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("PostProcess DepthStencilState");
    }

    SRHIRasterizerStateDesc RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("PostProcess RasterizerState");
    }

    SRHIBlendStateDesc BlendStateInfo;
    BlendStateInfo.bIndependentBlendEnable      = false;
    BlendStateInfo.RenderTarget[0].bBlendEnable = false;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("PostProcess BlendState");
    }

    SRHIGraphicsPipelineStateDesc PSOProperties;
    PSOProperties.InputLayoutState                       = nullptr;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PostShader.Get();
    PSOProperties.PrimitiveTopologyType                  = ERHIPrimitiveTopologyType::Triangle;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = Resources.BackBufferFormat;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = ERHIFormat::Unknown;

    PostPSO = RHICreateGraphicsPipelineState(PSOProperties);
    if (!PostPSO)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PostPSO->SetName("PostProcess PipelineState");
    }

    // FXAA
    CRHISamplerStateDesc CreateInfo;
    CreateInfo.AddressU = ERHISamplerMode::Clamp;
    CreateInfo.AddressV = ERHISamplerMode::Clamp;
    CreateInfo.AddressW = ERHISamplerMode::Clamp;
    CreateInfo.Filter   = ERHISamplerFilter::MinMagMipLinear;

    Resources.FXAASampler = RHICreateSamplerState(CreateInfo);
    if (!Resources.FXAASampler)
    {
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/FXAA_PS.hlsl", "Main", nullptr, ERHIShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        FXAAShader->SetName("FXAA PixelShader");
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

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/FXAA_PS.hlsl", "Main", &Defines, ERHIShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        FXAADebugShader->SetName("FXAA PixelShader");
    }

    PSOProperties.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = RHICreateGraphicsPipelineState(PSOProperties);
    if (!FXAADebugPSO)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FXAADebugPSO->SetName("FXAA Debug PipelineState");
    }

    return true;
}

bool CRenderer::InitShadingImage()
{
    SRHIShadingRateSupport Support;
    RHICheckShadingRateSupport(Support);

    if (Support.Tier != ERHIShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0)
    {
        return true;
    }

    uint32 Width  = Resources.MainWindowViewport->GetWidth() / Support.ShadingRateImageTileSize;
    uint32 Height = Resources.MainWindowViewport->GetHeight() / Support.ShadingRateImageTileSize;
    ShadingImage = RHICreateTexture2D(ERHIFormat::R8_Uint, Width, Height, 1, 1, TextureFlags_RWTexture, ERHIResourceState::ShadingRateSource, nullptr);
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
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ShadingImage.hlsl", "Main", nullptr, ERHIShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        ShadingRateShader->SetName("ShadingRate Image Shader");
    }

    SRHIComputePipelineStateDesc CreateInfo(ShadingRateShader.Get());
    ShadingRatePipeline = RHICreateComputePipelineState(CreateInfo);
    if (!ShadingRatePipeline)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRatePipeline->SetName("ShadingRate Image Pipeline");
    }

    return true;
}
