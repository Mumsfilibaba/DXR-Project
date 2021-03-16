#include "Renderer.h"
#include "DebugUI.h"

#include "Resources/TextureFactory.h"
#include "Resources/Mesh.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Application/Events/EventDispatcher.h"

#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include <algorithm>
#include <imgui_internal.h>

static const UInt32 ShadowMapSampleCount = 2;

ConsoleVariable gDrawTextureDebugger(EConsoleVariableType::Bool);
ConsoleVariable gDrawRendererInfo(EConsoleVariableType::Bool);

ConsoleVariable gEnableSSAO(EConsoleVariableType::Bool);
ConsoleVariable gEnableFXAA(EConsoleVariableType::Bool);
ConsoleVariable gEnableVariableRateShading(EConsoleVariableType::Bool);

ConsoleVariable gPrePassEnabled(EConsoleVariableType::Bool);
ConsoleVariable gDrawAABBs(EConsoleVariableType::Bool);
ConsoleVariable gVSyncEnabled(EConsoleVariableType::Bool);
ConsoleVariable gFrustumCullEnabled(EConsoleVariableType::Bool);
ConsoleVariable gRayTracingEnabled(EConsoleVariableType::Bool);

ConsoleVariable gFXAADebug(EConsoleVariableType::Bool);

void Renderer::PerformFrustumCulling(const Scene& Scene)
{
    TRACE_SCOPE("Frustum Culling");

    Camera* Camera        = Scene.GetCamera();
    Frustum CameraFrustum = Frustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
    {
        const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
        XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
        XMVECTOR XmTop       = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
        XMVECTOR XmBottom    = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
        XmTop    = XMVector4Transform(XmTop, XmTransform);
        XmBottom = XMVector4Transform(XmBottom, XmTransform);

        AABB Box;
        XMStoreFloat3(&Box.Top, XmTop);
        XMStoreFloat3(&Box.Bottom, XmBottom);
        if (CameraFrustum.CheckAABB(Box))
        {
            if (Command.Material->HasAlphaMask())
            {
                Resources.ForwardVisibleCommands.EmplaceBack(Command);
            }
            else
            {
                Resources.DeferredVisibleCommands.EmplaceBack(Command);
            }
        }
    }
}

void Renderer::PerformFXAA(CommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin FXAA");

    TRACE_SCOPE("FXAA");
    
    struct FXAASettings
    {
        Float Width;
        Float Height;
    } Settings;

    Settings.Width  = static_cast<Float>(Resources.BackBuffer->GetWidth());
    Settings.Height = static_cast<Float>(Resources.BackBuffer->GetHeight());

    RenderTargetView* BackBufferRTV = Resources.BackBuffer->GetRenderTargetView();
    InCmdList.SetRenderTargets(&BackBufferRTV, 1, nullptr);

    ShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    if (gFXAADebug.GetBool())
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

void Renderer::PerformBackBufferBlit(CommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin Draw BackBuffer");

    TRACE_SCOPE("Draw to BackBuffer");

    RenderTargetView* BackBufferRTV = Resources.BackBuffer->GetRenderTargetView();
    InCmdList.SetRenderTargets(&BackBufferRTV, 1, nullptr);

    ShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    InCmdList.SetShaderResourceView(PostShader.Get(), FinalTargetSRV, 0);
    InCmdList.SetSamplerState(PostShader.Get(), Resources.GBufferSampler.Get(), 0);

    InCmdList.SetGraphicsPipelineState(PostPSO.Get());
    InCmdList.DrawInstanced(3, 1, 0, 0);

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End Draw BackBuffer");
}

void Renderer::PerformAABBDebugPass(CommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin DebugPass");

    TRACE_SCOPE("DebugPass");

    InCmdList.SetGraphicsPipelineState(AABBDebugPipelineState.Get());

    InCmdList.SetPrimitiveTopology(EPrimitiveTopology::LineList);

    InCmdList.SetConstantBuffer(AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0);

    InCmdList.SetVertexBuffers(&AABBVertexBuffer, 1, 0);
    InCmdList.SetIndexBuffer(AABBIndexBuffer.Get());

    for (const MeshDrawCommand& Command : Resources.DeferredVisibleCommands)
    {
        AABB& Box = Command.Mesh->BoundingBox;
        XMFLOAT3 Scale    = XMFLOAT3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        XMFLOAT3 Position = Box.GetCenter();

        XMMATRIX XmTranslation = XMMatrixTranslation(Position.x, Position.y, Position.z);
        XMMATRIX XmScale       = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

        XMFLOAT4X4 Transform   = Command.CurrentActor->GetTransform().GetMatrix();
        XMMATRIX   XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
        XMStoreFloat4x4(&Transform, XMMatrixMultiplyTranspose(XMMatrixMultiply(XmScale, XmTranslation), XmTransform));

        InCmdList.Set32BitShaderConstants(AABBVertexShader.Get(), &Transform, 16);

        InCmdList.DrawIndexedInstanced(24, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End DebugPass");
}

void Renderer::RenderDebugInterface()
{
    if (gDrawTextureDebugger.GetBool())
    {
        constexpr Float InvAspectRatio = 16.0f / 9.0f;
        constexpr Float AspectRatio    = 9.0f / 16.0f;

        const UInt32 WindowWidth  = gMainWindow->GetWidth();
        const UInt32 WindowHeight = gMainWindow->GetHeight();
        const Float Width  = Math::Max(WindowWidth * 0.6f, 400.0f);
        const Float Height = WindowHeight * 0.75f;

        ImGui::SetNextWindowPos(ImVec2(Float(WindowWidth) * 0.5f, Float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags =
            ImGuiWindowFlags_NoResize           |
            ImGuiWindowFlags_NoScrollbar        |
            ImGuiWindowFlags_NoCollapse         |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings;

        Bool TempDrawTextureDebugger = gDrawTextureDebugger.GetBool();
        if (ImGui::Begin("FrameBuffer Debugger", &TempDrawTextureDebugger, Flags))
        {
            ImGui::BeginChild("##ScrollBox", ImVec2(Width * 0.985f, Height * 0.125f), true, ImGuiWindowFlags_HorizontalScrollbar);

            const Int32 Count = Resources.DebugTextures.Size();
            static Int32 SelectedImage = -1;
            if (SelectedImage >= Count)
            {
                SelectedImage = -1;
            }

            for (Int32 i = 0; i < Count; i++)
            {
                ImGui::PushID(i);

                constexpr Float MenuImageSize = 96.0f;
                Int32  FramePadding = 2;
                ImVec2 Size    = ImVec2(MenuImageSize * InvAspectRatio, MenuImageSize);
                ImVec2 Uv0     = ImVec2(0.0f, 0.0f);
                ImVec2 Uv1     = ImVec2(1.0f, 1.0f);
                ImVec4 BgCol   = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                ImVec4 TintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                ImGuiImage* CurrImage = &Resources.DebugTextures[i];
                if (ImGui::ImageButton(CurrImage, Size, Uv0, Uv1, FramePadding, BgCol, TintCol))
                {
                    SelectedImage = i;
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", CurrImage->Image->GetName().c_str());
                }

                ImGui::PopID();

                if (i != Count - 1)
                {
                    ImGui::SameLine();
                }
            }

            ImGui::EndChild();

            const Float ImageWidth  = Width * 0.985f;
            const Float ImageHeight = ImageWidth * AspectRatio;
            const Int32 ImageIndex  = SelectedImage < 0 ? 0 : SelectedImage;
            ImGuiImage* CurrImage   = &Resources.DebugTextures[ImageIndex];
            ImGui::Image(CurrImage, ImVec2(ImageWidth, ImageHeight));
        }

        ImGui::End();

        gDrawTextureDebugger.SetBool(TempDrawTextureDebugger);
    }

    if (gDrawRendererInfo.GetBool())
    {
        const UInt32 WindowWidth  = gMainWindow->GetWidth();
        const UInt32 WindowHeight = gMainWindow->GetHeight();
        const Float Width  = 300.0f;
        const Float Height = WindowHeight * 0.8f;

        ImGui::SetNextWindowPos(ImVec2(Float(WindowWidth), 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Always);

        const ImGuiWindowFlags Flags = 
            ImGuiWindowFlags_NoMove             | 
            ImGuiWindowFlags_NoDecoration       | 
            ImGuiWindowFlags_NoFocusOnAppearing | 
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("Renderer Window", nullptr, Flags);

        ImGui::Text("Renderer Status:");
        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 100.0f);

        const std::string AdapterName = GetAdapterName();
        ImGui::Text("Adapter: ");
        ImGui::NextColumn();

        ImGui::Text("%s", AdapterName.c_str());
        ImGui::NextColumn();

        ImGui::Text("DrawCalls: ");
        ImGui::NextColumn();

        ImGui::Text("%d", LastFrameNumDrawCalls);
        ImGui::NextColumn();

        ImGui::Text("DispatchCalls: ");
        ImGui::NextColumn();

        ImGui::Text("%d", LastFrameNumDispatchCalls);
        ImGui::NextColumn();

        ImGui::Text("Command Count: ");
        ImGui::NextColumn();

        ImGui::Text("%d", LastFrameNumCommands);

        ImGui::Columns(1);

        ImGui::End();
    }
}

void Renderer::Tick(const Scene& Scene)
{
    Resources.BackBuffer = Resources.MainWindowViewport->GetBackBuffer();

    Camera* CurrentCamera = Scene.GetCamera();
    CurrentCamera->Tick(Resources.BackBuffer->GetWidth(), Resources.BackBuffer->GetHeight());

    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();
    Resources.DebugTextures.Clear();

    if (!gFrustumCullEnabled.GetBool())
    {
        for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
        {
            if (Command.Material->HasAlphaMask())
            {
                Resources.ForwardVisibleCommands.EmplaceBack(Command);
            }
            else
            {
                Resources.DeferredVisibleCommands.EmplaceBack(Command);
            }
        }
    }
    else
    {
        PerformFrustumCulling(Scene);
    }

    CmdList.BeginExternalCapture();
    CmdList.Begin();

    Profiler::BeginGPUFrame(CmdList);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--BEGIN FRAME--");

    if (ShadingImage && gEnableVariableRateShading.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin VRS Image");
        CmdList.SetShadingRate(EShadingRate::VRS_1x1);

        CmdList.TransitionTexture(ShadingImage.Get(), EResourceState::ShadingRateSource, EResourceState::UnorderedAccess);
        
        CmdList.SetComputePipelineState(ShadingRatePipeline.Get());

        UnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        CmdList.SetUnorderedAccessView(ShadingRateShader.Get(), ShadingImageUAV, 0);
        
        CmdList.Dispatch(ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1);
        
        CmdList.TransitionTexture(ShadingImage.Get(), EResourceState::UnorderedAccess, EResourceState::ShadingRateSource);

        CmdList.SetShadingRateImage(ShadingImage.Get());

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End VRS Image");
    }
    else
    {
        CmdList.SetShadingRate(EShadingRate::VRS_1x1);
    }

    LightSetup.BeginFrame(CmdList, Scene);

    ShadowMapRenderer.RenderPointLightShadows(CmdList, LightSetup, Scene);
    ShadowMapRenderer.RenderDirectionalLightShadows(CmdList, LightSetup, Scene);

    // Update camerabuffer
    CameraBufferData.PrevViewProjection = CameraBufferData.ViewProjection;
    CameraBufferData.ViewProjection     = CurrentCamera->GetViewProjectionMatrix();
    CameraBufferData.View               = CurrentCamera->GetViewMatrix();
    CameraBufferData.Projection         = CurrentCamera->GetProjectionMatrix();
    CameraBufferData.ViewInv            = CurrentCamera->GetViewInverseMatrix();
    CameraBufferData.ProjectionInv      = CurrentCamera->GetProjectionInverseMatrix();
    CameraBufferData.ViewProjectionInv  = CurrentCamera->GetViewProjectionInverseMatrix();
    CameraBufferData.Position           = CurrentCamera->GetPosition();
    CameraBufferData.Forward            = CurrentCamera->GetForward();
    CameraBufferData.NearPlane          = CurrentCamera->GetNearPlane();
    CameraBufferData.FarPlane           = CurrentCamera->GetFarPlane();
    CameraBufferData.AspectRatio        = CurrentCamera->GetAspectRatio();
    CameraBufferData.PrevJitter         = CameraBufferData.Jitter;
    CameraBufferData.Jitter             = CurrentCamera->GetJitter();
    CameraBufferData.Width              = Resources.BackBuffer->GetWidth();
    CameraBufferData.Height             = Resources.BackBuffer->GetHeight();

    CmdList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

    CmdList.UpdateBuffer(Resources.CameraBuffer.Get(), 0, sizeof(CameraBufferDesc), &CameraBufferData);
    
    CmdList.TransitionBuffer(Resources.CameraBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    
    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget);
    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget);
    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget);
    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget);
    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VELOCITY_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget);
    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite);

    ColorF BlackClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    CmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBuffer[GBUFFER_VELOCITY_INDEX]->GetRenderTargetView(), BlackClearColor);
    CmdList.ClearDepthStencilView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView(), DepthStencilF(1.0f, 0));

    if (gPrePassEnabled.GetBool())
    {
        GPU_TRACE_SCOPE(CmdList, "Pre Pass");
        DeferredRenderer.RenderPrePass(CmdList, Resources);
    }

    {
        GPU_TRACE_SCOPE(CmdList, "Base Pass");
        DeferredRenderer.RenderBasePass(CmdList, Resources);
    }

    if (IsRayTracingSupported())
    {
        GPU_TRACE_SCOPE(CmdList, "Ray Tracing");
        RayTracer.Render(CmdList, Resources, LightSetup, Scene);
    }

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource);

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_NORMAL_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource);

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource);

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_VELOCITY_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.GBuffer[GBUFFER_VELOCITY_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_VELOCITY_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource);

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource);

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess);

    const ColorF WhiteColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    CmdList.ClearUnorderedAccessView(Resources.SSAOBuffer->GetUnorderedAccessView(), WhiteColor);

    if (gEnableSSAO.GetBool())
    {
        SSAORenderer.Render(CmdList, Resources);
    }

    CmdList.TransitionTexture(Resources.SSAOBuffer.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.SSAOBuffer->GetShaderResourceView()),
        Resources.SSAOBuffer, 
        EResourceState::NonPixelShaderResource, 
        EResourceState::NonPixelShaderResource);

    CmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceState::PixelShaderResource, EResourceState::UnorderedAccess);
    CmdList.TransitionTexture(Resources.BackBuffer, EResourceState::Present, EResourceState::RenderTarget);
    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);

    {
        GPU_TRACE_SCOPE(CmdList, "Light Pass");
        DeferredRenderer.RenderDeferredTiledLightPass(CmdList, Resources, LightSetup);
    }

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite);
    CmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceState::UnorderedAccess, EResourceState::RenderTarget);

    SkyboxRenderPass.Render(CmdList, Resources, Scene);

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.DirLightShadowMaps.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(LightSetup.DirLightShadowMaps->GetShaderResourceView()),
        LightSetup.DirLightShadowMaps,
        EResourceState::PixelShaderResource,
        EResourceState::PixelShaderResource);

    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(Resources.IntegrationLUT.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.IntegrationLUT->GetShaderResourceView()),
        Resources.IntegrationLUT,
        EResourceState::PixelShaderResource,
        EResourceState::PixelShaderResource);

    {
        GPU_TRACE_SCOPE(CmdList, "Forward Pass");
        ForwardRenderer.Render(CmdList, Resources, LightSetup);
    }
    
    CmdList.TransitionTexture(Resources.FinalTarget.Get(), EResourceState::RenderTarget, EResourceState::PixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.FinalTarget->GetShaderResourceView()),
        Resources.FinalTarget, 
        EResourceState::PixelShaderResource, 
        EResourceState::PixelShaderResource);

    CmdList.TransitionTexture(Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::DepthWrite, EResourceState::PixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView()),
        Resources.GBuffer[GBUFFER_DEPTH_INDEX],
        EResourceState::PixelShaderResource,
        EResourceState::PixelShaderResource);

    if (gEnableFXAA.GetBool())
    {
        PerformFXAA(CmdList);
    }
    else
    {
        PerformBackBufferBlit(CmdList);
    }

    if (gDrawAABBs.GetBool())
    {
        PerformAABBDebugPass(CmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin UI Render");

    {
        TRACE_SCOPE("Render UI");

        DebugUI::DrawUI([]()
        {
            gRenderer.RenderDebugInterface();
        });

        CmdList.SetShadingRate(EShadingRate::VRS_1x1);
        CmdList.SetShadingRateImage(nullptr);

        DebugUI::Render(CmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End UI Render");

    CmdList.TransitionTexture(Resources.BackBuffer, EResourceState::RenderTarget, EResourceState::Present);
    
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--END FRAME--");

    Profiler::EndGPUFrame(CmdList);

    CmdList.End();
    CmdList.EndExternalCapture();

    LastFrameNumDrawCalls     = CmdList.GetNumDrawCalls();
    LastFrameNumDispatchCalls = CmdList.GetNumDispatchCalls();
    LastFrameNumCommands      = CmdList.GetNumCommands();

    {
        TRACE_SCOPE("ExecuteCommandList");
        gCmdListExecutor.ExecuteCommandList(CmdList);
    }

    {
        TRACE_SCOPE("Present");
        Resources.MainWindowViewport->Present(gVSyncEnabled.GetBool());
    }
}

Bool Renderer::Init()
{
    INIT_CONSOLE_VARIABLE("r.DrawTextureDebugger", gDrawTextureDebugger);
    gDrawTextureDebugger.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.DrawRendererInfo", gDrawRendererInfo);
    gDrawRendererInfo.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnableSSAO", gEnableSSAO);
    gEnableSSAO.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableFXAA", gEnableFXAA);
    gEnableFXAA.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableVariableRateShading", gEnableVariableRateShading);
    gEnableVariableRateShading.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnablePrePass", gPrePassEnabled);
    gPrePassEnabled.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableDrawAABBs", gDrawAABBs);
    gDrawAABBs.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnableVerticalSync", gVSyncEnabled);
    gVSyncEnabled.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnableFrustumCulling", gFrustumCullEnabled);
    gFrustumCullEnabled.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableRayTracing", gRayTracingEnabled);
    gRayTracingEnabled.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.FXAADebug", gFXAADebug);
    gFXAADebug.SetBool(false);

    Resources.MainWindowViewport = CreateViewport(gMainWindow, 0, 0, EFormat::R8G8B8A8_Unorm, EFormat::Unknown);
    if (!Resources.MainWindowViewport)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    Resources.CameraBuffer = CreateConstantBuffer<CameraBufferDesc>(BufferFlag_Default, EResourceState::Common, nullptr);
    if (!Resources.CameraBuffer)
    {
        LOG_ERROR("[Renderer]: Failed to create camerabuffer");
        return false;
    }
    else
    {
        Resources.CameraBuffer->SetName("CameraBuffer");
    }

    // Init standard inputlayout
    InputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, 0, 0,  EInputClassification::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, 0, 12, EInputClassification::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, 0, 24, EInputClassification::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    0, 36, EInputClassification::Vertex, 0 },
    };

    Resources.StdInputLayout = CreateInputLayout(InputLayout);
    if (!Resources.StdInputLayout)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.StdInputLayout->SetName("Standard InputLayoutState");
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU       = ESamplerMode::Border;
        CreateInfo.AddressV       = ESamplerMode::Border;
        CreateInfo.AddressW       = ESamplerMode::Border;
        CreateInfo.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        CreateInfo.ComparisonFunc = EComparisonFunc::LessEqual;
        CreateInfo.BorderColor    = ColorF(1.0f, 1.0f, 1.0f, 1.0f);

        Resources.DirectionalShadowSampler = CreateSamplerState(CreateInfo);
        if (!Resources.DirectionalShadowSampler)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            Resources.DirectionalShadowSampler->SetName("ShadowMap Sampler");
        }
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU       = ESamplerMode::Wrap;
        CreateInfo.AddressV       = ESamplerMode::Wrap;
        CreateInfo.AddressW       = ESamplerMode::Wrap;
        CreateInfo.Filter         = ESamplerFilter::Comparison_MinMagMipLinear;
        CreateInfo.ComparisonFunc = EComparisonFunc::LessEqual;

        Resources.PointShadowSampler = CreateSamplerState(CreateInfo);
        if (!Resources.PointShadowSampler)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            Resources.PointShadowSampler->SetName("ShadowMap Comparison Sampler");
        }
    }

    GPUProfiler = CreateProfiler();
    if (!GPUProfiler)
    {
        return false;
    }

    Profiler::SetGPUProfiler(GPUProfiler.Get());

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

    if (IsRayTracingSupported())
    {
        if (!RayTracer.Init(Resources))
        {
            return false;
        }
    }

    if (!InitBlueNoise())
    {
        return false;
    }

    CmdList.Begin();

    LightProbeRenderer.RenderSkyLightProbe(CmdList, LightSetup, Resources);

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    auto Callback = [](const Event& Event)->Bool
    {
        if (!IsEventOfType<WindowResizeEvent>(Event))
        {
            return false;
        }

        const WindowResizeEvent& ResizeEvent = CastEvent<WindowResizeEvent>(Event);
        gRenderer.ResizeResources(ResizeEvent.Width, ResizeEvent.Height);

        return true;
    };

    // Register EventFunc
    gEventDispatcher->RegisterEventHandler(Callback, EEventCategory::EventCategory_Window);

    return true;
}

void Renderer::Release()
{
    gCmdListExecutor.WaitForGPU();

    CmdList.Reset();

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

    GPUProfiler.Reset();
    Profiler::SetGPUProfiler(nullptr);

    LastFrameNumDrawCalls     = 0;
    LastFrameNumDispatchCalls = 0;
    LastFrameNumCommands      = 0;
}

Bool Renderer::InitBoundingBoxDebugPass()
{
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Debug.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    AABBVertexShader = CreateVertexShader(ShaderCode);
    if (!AABBVertexShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBVertexShader->SetName("Debug VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Debug.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    AABBPixelShader = CreatePixelShader(ShaderCode);
    if (!AABBPixelShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBPixelShader->SetName("Debug PixelShader");
    }

    InputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, 0, 0, EInputClassification::Vertex, 0 },
    };

    TRef<InputLayoutState> InputLayoutState = CreateInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        InputLayoutState->SetName("Debug InputLayoutState");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("Debug DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Debug RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;

    TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Debug BlendState");
    }

    GraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.InputLayoutState                       = InputLayoutState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = AABBVertexShader.Get();
    PSOProperties.ShaderState.PixelShader                = AABBPixelShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Line;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = Resources.RenderTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = Resources.DepthBufferFormat;

    AABBDebugPipelineState = CreateGraphicsPipelineState(PSOProperties);
    if (!AABBDebugPipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBDebugPipelineState->SetName("Debug PipelineState");
    }

    TStaticArray<XMFLOAT3, 8> Vertices =
    {
        XMFLOAT3(-0.5f, -0.5f,  0.5f),
        XMFLOAT3( 0.5f, -0.5f,  0.5f),
        XMFLOAT3(-0.5f,  0.5f,  0.5f),
        XMFLOAT3( 0.5f,  0.5f,  0.5f),

        XMFLOAT3( 0.5f, -0.5f, -0.5f),
        XMFLOAT3(-0.5f, -0.5f, -0.5f),
        XMFLOAT3( 0.5f,  0.5f, -0.5f),
        XMFLOAT3(-0.5f,  0.5f, -0.5f)
    };

    ResourceData VertexData(Vertices.Data(), Vertices.SizeInBytes());

    AABBVertexBuffer = CreateVertexBuffer<XMFLOAT3>(Vertices.Size(), BufferFlag_Default, EResourceState::Common, &VertexData);
    if (!AABBVertexBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBVertexBuffer->SetName("AABB VertexBuffer");
    }

    // Create IndexBuffer
    TStaticArray<UInt16, 24> Indices =
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

    ResourceData IndexData(Indices.Data(), Indices.SizeInBytes());

    AABBIndexBuffer = CreateIndexBuffer(EIndexFormat::UInt16, Indices.Size(), BufferFlag_Default, EResourceState::Common, &IndexData);
    if (!AABBIndexBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBIndexBuffer->SetName("AABB IndexBuffer");
    }

    return true;
}

Bool Renderer::InitAA()
{
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/FullscreenVS.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TRef<VertexShader> VShader = CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Fullscreen VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/PostProcessPS.hlsl", "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    PostShader = CreatePixelShader(ShaderCode);
    if (!PostShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PostShader->SetName("PostProcess PixelShader");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::Always;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("PostProcess DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("PostProcess RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable      = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = false;

    TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("PostProcess BlendState");
    }

    GraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.InputLayoutState                       = nullptr;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PostShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    PostPSO = CreateGraphicsPipelineState(PSOProperties);
    if (!PostPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PostPSO->SetName("PostProcess PipelineState");
    }

    // FXAA
    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    Resources.FXAASampler = CreateSamplerState(CreateInfo);
    if (!Resources.FXAASampler)
    {
        return false;
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/FXAA_PS.hlsl", "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    FXAAShader = CreatePixelShader(ShaderCode);
    if (!FXAAShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FXAAShader->SetName("FXAA PixelShader");
    }

    PSOProperties.ShaderState.PixelShader = FXAAShader.Get();

    FXAAPSO = CreateGraphicsPipelineState(PSOProperties);
    if (!FXAAPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FXAAPSO->SetName("FXAA PipelineState");
    }

    TArray<ShaderDefine> Defines =
    {
        ShaderDefine("ENABLE_DEBUG", "1")
    };

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/FXAA_PS.hlsl", "Main", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    FXAADebugShader = CreatePixelShader(ShaderCode);
    if (!FXAADebugShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FXAADebugShader->SetName("FXAA PixelShader");
    }

    PSOProperties.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = CreateGraphicsPipelineState(PSOProperties);
    if (!FXAADebugPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FXAADebugPSO->SetName("FXAA Debug PipelineState");
    }

    return true;
}

Bool Renderer::InitShadingImage()
{
    ShadingRateSupport Support;
    CheckShadingRateSupport(Support);

    if (Support.Tier != EShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0)
    {
        return true;
    }

    UInt32 Width  = Resources.MainWindowViewport->GetWidth()  / Support.ShadingRateImageTileSize;
    UInt32 Height = Resources.MainWindowViewport->GetHeight() / Support.ShadingRateImageTileSize;
    ShadingImage = CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::ShadingRateSource, nullptr);
    if (!ShadingImage)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadingImage->SetName("Shading Rate Image");
    }

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadingImage.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    ShadingRateShader = CreateComputeShader(ShaderCode);
    if (!ShadingRateShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRateShader->SetName("ShadingRate Image Shader");
    }

    ComputePipelineStateCreateInfo CreateInfo(ShadingRateShader.Get());
    ShadingRatePipeline = CreateComputePipelineState(CreateInfo);
    if (!ShadingRatePipeline)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRatePipeline->SetName("ShadingRate Image Pipeline");
    }

    return true;
}

Bool Renderer::InitBlueNoise()
{
    TArray<TRef<Texture2D>> TextureSlices;
    for (UInt32 i = 0; i < 64; i++)
    {
        std::string FileName = "../Assets/Textures/Blue Noise/LDR_RGBA_" + std::to_string(i) + ".png";
        
        TRef<Texture2D> Slice = TextureFactory::LoadFromFile(FileName, TextureFactoryFlag_None, EFormat::R8G8B8A8_Unorm);
        if (!Slice)
        {
            return false;
        }
        else
        {
            TextureSlices.EmplaceBack(Slice);
        }
    }

    Assert(TextureSlices[0] != nullptr);

    UInt32 Width  = TextureSlices[0]->GetWidth();
    UInt32 Height = TextureSlices[0]->GetHeight();
    Resources.BlueNoise = CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, Width, Height, 1, 1, 64, ETextureFlags::TextureFlag_SRV, EResourceState::CopyDest, nullptr);
    if (!Resources.BlueNoise)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.BlueNoise->SetName("Blue Noise Texture Array");
    }

    CmdList.Begin();
    
    CopyTextureInfo CopyInfo;
    CopyInfo.Width  = Width;
    CopyInfo.Height = Height;
    CopyInfo.Depth  = 1;
    CopyInfo.Source.x   = 0;
    CopyInfo.Source.y   = 0;
    CopyInfo.Source.z   = 0;
    CopyInfo.Source.Mip = 0;
    CopyInfo.Source.ArraySlice = 0;
    CopyInfo.Destination.x   = 0;
    CopyInfo.Destination.y   = 0;
    CopyInfo.Destination.z   = 0;
    CopyInfo.Destination.Mip = 0;

    for (UInt32 i = 0; i < 64; i++)
    {
        CmdList.TransitionTexture(TextureSlices[i].Get(), EResourceState::PixelShaderResource, EResourceState::CopySource);

        CopyInfo.Destination.ArraySlice = i;
        CmdList.CopyTextureRegion(Resources.BlueNoise.Get(), TextureSlices[i].Get(), CopyInfo);
    }

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    return true;
}

void Renderer::ResizeResources(UInt32 Width, UInt32 Height)
{
    if (!Resources.MainWindowViewport->Resize(Width, Height))
    {
        Debug::DebugBreak();
        return;
    }

    if (!DeferredRenderer.ResizeResources(Resources))
    {
        Debug::DebugBreak();
        return;
    }

    if (!SSAORenderer.ResizeResources(Resources))
    {
        Debug::DebugBreak();
        return;
    }

    if (!RayTracer.OnResize(Resources))
    {
        Debug::DebugBreak();
        return;
    }
}
