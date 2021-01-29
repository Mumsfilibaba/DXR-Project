#include "Renderer.h"
#include "TextureFactory.h"
#include "DebugUI.h"
#include "Mesh.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Application/Events/EventDispatcher.h"

#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include <algorithm>
#include <imgui_internal.h>

static const UInt32  ShadowMapSampleCount = 2;

ConsoleVariable GlobalDrawTextureDebugger(ConsoleVariableType_Bool);
ConsoleVariable GlobalDrawRendererInfo(ConsoleVariableType_Bool);

ConsoleVariable GlobalEnableSSAO(ConsoleVariableType_Bool);
ConsoleVariable GlobalEnableFXAA(ConsoleVariableType_Bool);

ConsoleVariable GlobalPrePassEnabled(ConsoleVariableType_Bool);
ConsoleVariable GlobalDrawAABBs(ConsoleVariableType_Bool);
ConsoleVariable GlobalVSyncEnabled(ConsoleVariableType_Bool);
ConsoleVariable GlobalFrustumCullEnabled(ConsoleVariableType_Bool);
ConsoleVariable GlobalRayTracingEnabled(ConsoleVariableType_Bool);

struct CameraBufferDesc
{
    XMFLOAT4X4 ViewProjection;
    XMFLOAT4X4 View;
    XMFLOAT4X4 ViewInv;
    XMFLOAT4X4 Projection;
    XMFLOAT4X4 ProjectionInv;
    XMFLOAT4X4 ViewProjectionInv;
    XMFLOAT3   Position;
    Float      NearPlane;
    Float      FarPlane;
    Float      AspectRatio;
};

void Renderer::PerformFrustumCulling(const Scene& Scene)
{
    TRACE_SCOPE("Frustum Culling");

    Camera* Camera        = Scene.GetCamera();
    Frustum CameraFrustum = Frustum(
        Camera->GetFarPlane(), 
        Camera->GetViewMatrix(), 
        Camera->GetProjectionMatrix());
    
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
    
    InCmdList.BindRenderTargets(&Resources.BackBufferRTV, 1, nullptr);

    InCmdList.BindShaderResourceViews(
        EShaderStage::ShaderStage_Pixel,
        &Resources.FinalTargetSRV, 1, 0);

    InCmdList.BindSamplerStates(
        EShaderStage::ShaderStage_Pixel,
        &Resources.GBufferSampler, 1, 0);

    struct FXAASettings
    {
        Float Width;
        Float Height;
    } Settings;

    Settings.Width  = static_cast<Float>(Resources.BackBuffer->GetWidth());
    Settings.Height = static_cast<Float>(Resources.BackBuffer->GetHeight());

    InCmdList.Bind32BitShaderConstants(
        EShaderStage::ShaderStage_Pixel,
        &Settings, 2);

    InCmdList.BindGraphicsPipelineState(FXAAPSO.Get());

    InCmdList.DrawInstanced(3, 1, 0, 0);

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End FXAA");
}

void Renderer::PerformBackBufferBlit(CommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin Draw BackBuffer");

    TRACE_SCOPE("Draw to BackBuffer");

    InCmdList.BindRenderTargets(&Resources.BackBufferRTV, 1, nullptr);

    InCmdList.BindShaderResourceViews(
        EShaderStage::ShaderStage_Pixel,
        &Resources.FinalTargetSRV, 1, 0);

    InCmdList.BindSamplerStates(
        EShaderStage::ShaderStage_Pixel,
        &Resources.GBufferSampler, 1, 0);

    InCmdList.BindGraphicsPipelineState(PostPSO.Get());
    InCmdList.DrawInstanced(3, 1, 0, 0);

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End Draw BackBuffer");
}

void Renderer::PerformAABBDebugPass(CommandList& InCmdList)
{
    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "Begin DebugPass");

    TRACE_SCOPE("DebugPass");

    InCmdList.BindGraphicsPipelineState(AABBDebugPipelineState.Get());

    InCmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_LineList);

    InCmdList.BindConstantBuffers(
        EShaderStage::ShaderStage_Vertex,
        &Resources.CameraBuffer,
        1, 0);

    InCmdList.BindVertexBuffers(&AABBVertexBuffer, 1, 0);
    InCmdList.BindIndexBuffer(AABBIndexBuffer.Get());

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

        InCmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Vertex,
            &Transform, 16);

        InCmdList.DrawIndexedInstanced(24, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(InCmdList, "End DebugPass");
}

void Renderer::RenderDebugInterface()
{
    if (GlobalDrawTextureDebugger.GetBool())
    {
        Resources.DebugTextures.EmplaceBack(
            Resources.GBufferSRVs[GBUFFER_DEPTH_INDEX],
            Resources.GBuffer[GBUFFER_DEPTH_INDEX],
            EResourceState::ResourceState_DepthWrite,
            EResourceState::ResourceState_PixelShaderResource);

        constexpr Float InvAspectRatio = 16.0f / 9.0f;
        constexpr Float AspectRatio    = 9.0f / 16.0f;

        const UInt32 WindowWidth  = gMainWindow->GetWidth();
        const UInt32 WindowHeight = gMainWindow->GetHeight();
        const Float Width  = Math::Max(WindowWidth * 0.6f, 400.0f);
        const Float Height = WindowHeight * 0.75f;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

        ImGui::SetNextWindowPos(
            ImVec2(Float(WindowWidth) * 0.5f, Float(WindowHeight) * 0.175f),
            ImGuiCond_Appearing,
            ImVec2(0.5f, 0.0f));

        ImGui::SetNextWindowSize(
            ImVec2(Width, Height),
            ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags =
            ImGuiWindowFlags_NoResize           |
            ImGuiWindowFlags_NoScrollbar        |
            ImGuiWindowFlags_NoCollapse         |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings;

        Bool TempDrawTextureDebugger = GlobalDrawTextureDebugger.GetBool();
        if (ImGui::Begin(
            "FrameBuffer Debugger",
            &TempDrawTextureDebugger,
            Flags))
        {
            ImGui::BeginChild(
                "##ScrollBox",
                ImVec2(Width * 0.985f, Height * 0.125f),
                true,
                ImGuiWindowFlags_HorizontalScrollbar);

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

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }

        ImGui::End();

        GlobalDrawTextureDebugger.SetBool(TempDrawTextureDebugger);
    }
    else
    {
        CmdList.TransitionTexture(
            Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            EResourceState::ResourceState_DepthWrite,
            EResourceState::ResourceState_PixelShaderResource);
    }

    if (GlobalDrawRendererInfo.GetBool())
    {
        const UInt32 WindowWidth  = gMainWindow->GetWidth();
        const UInt32 WindowHeight = gMainWindow->GetHeight();
        const Float Width = 300.0f;
        const Float Height = WindowHeight * 0.1f;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

        ImGui::SetNextWindowPos(
            ImVec2(Float(WindowWidth), 10.0f),
            ImGuiCond_Always,
            ImVec2(1.0f, 0.0f));

        ImGui::SetNextWindowSize(
            ImVec2(Width, Height),
            ImGuiCond_Always);

        ImGui::Begin(
            "Renderer Window",
            nullptr,
            ImGuiWindowFlags_NoMove             |
            ImGuiWindowFlags_NoDecoration       |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("Renderer Status:");
        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 100.0f);

        const std::string AdapterName = RenderLayer::GetAdapterName();
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

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::End();
    }
}

void Renderer::Tick(const Scene& Scene)
{
    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();
    Resources.DebugTextures.Clear();

    if (!GlobalFrustumCullEnabled.GetBool())
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

    Resources.BackBuffer    = Resources.MainWindowViewport->GetBackBuffer();
    Resources.BackBufferRTV = Resources.MainWindowViewport->GetRenderTargetView();

    CmdList.Begin();
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--BEGIN FRAME--");

    CmdList.SetShadingRate(EShadingRate::ShadingRate_1x1);

    ShadowMapRenderer.RenderPointLightShadows(
        CmdList,
        LightSetup,
        Scene);

    ShadowMapRenderer.RenderDirectionalLightShadows(
        CmdList,
        LightSetup,
        Scene);

    // Update camerabuffer
    CameraBufferDesc CamBuff;
    CamBuff.ViewProjection    = Scene.GetCamera()->GetViewProjectionMatrix();
    CamBuff.View              = Scene.GetCamera()->GetViewMatrix();
    CamBuff.ViewInv           = Scene.GetCamera()->GetViewInverseMatrix();
    CamBuff.Projection        = Scene.GetCamera()->GetProjectionMatrix();
    CamBuff.ProjectionInv     = Scene.GetCamera()->GetProjectionInverseMatrix();
    CamBuff.ViewProjectionInv = Scene.GetCamera()->GetViewProjectionInverseMatrix();
    CamBuff.Position          = Scene.GetCamera()->GetPosition();
    CamBuff.NearPlane         = Scene.GetCamera()->GetNearPlane();
    CamBuff.FarPlane          = Scene.GetCamera()->GetFarPlane();
    CamBuff.AspectRatio       = Scene.GetCamera()->GetAspectRatio();

    CmdList.TransitionBuffer(
        Resources.CameraBuffer.Get(), 
        EResourceState::ResourceState_VertexAndConstantBuffer, 
        EResourceState::ResourceState_CopyDest);

    CmdList.UpdateBuffer(Resources.CameraBuffer.Get(), 0, sizeof(CameraBufferDesc), &CamBuff);
    
    CmdList.TransitionBuffer(
        Resources.CameraBuffer.Get(),
        EResourceState::ResourceState_CopyDest, 
        EResourceState::ResourceState_VertexAndConstantBuffer);
    
    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    // Clear FrameResources.GBuffer
    ColorClearValue BlackClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    CmdList.ClearRenderTargetView(Resources.GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(Resources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearDepthStencilView(Resources.GBufferDSV.Get(), DepthStencilClearValue(1.0f, 0));

    if (GlobalPrePassEnabled.GetBool())
    {
        DeferredRenderer.RenderPrePass(
            CmdList,
            Resources);
    }

    DeferredRenderer.RenderBasePass(
        CmdList, 
        Resources);

    // Setup FrameResources.GBuffer for Read
    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        Resources.GBufferSRVs[GBUFFER_ALBEDO_INDEX],
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX],
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        Resources.GBufferSRVs[GBUFFER_NORMAL_INDEX],
        Resources.GBuffer[GBUFFER_NORMAL_INDEX],
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        Resources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX],
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        Resources.GBufferSRVs[GBUFFER_MATERIAL_INDEX],
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX],
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
        EResourceState::ResourceState_DepthWrite,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        Resources.SSAOBuffer.Get(), 
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_UnorderedAccess);

    const Float WhiteColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    CmdList.ClearUnorderedAccessView(Resources.SSAOBufferUAV.Get(), WhiteColor);

    if (GlobalEnableSSAO.GetBool())
    {
        SSAORenderer.Render(CmdList, Resources);
    }

    CmdList.TransitionTexture(
        Resources.SSAOBuffer.Get(),
        EResourceState::ResourceState_UnorderedAccess,
        EResourceState::ResourceState_NonPixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        Resources.SSAOBufferSRV,
        Resources.SSAOBuffer,
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    // Render to final output
    CmdList.TransitionTexture(
        Resources.FinalTarget.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_UnorderedAccess);
    
    CmdList.TransitionTexture(
        Resources.BackBuffer,
        EResourceState::ResourceState_Present, 
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        LightSetup.IrradianceMap.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        LightSetup.SpecularIrradianceMap.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        Resources.IntegrationLUT.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    DeferredRenderer.RenderDeferredTiledLightPass(
        CmdList,
        Resources,
        LightSetup);

    SkyboxRenderPass.Render(
        CmdList, 
        Resources,
        Scene);

    CmdList.TransitionTexture(
        Resources.FinalTarget.Get(), 
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_PixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        Resources.FinalTargetSRV,
        Resources.FinalTarget,
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        LightSetup.PointLightShadowMaps.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        LightSetup.DirLightShadowMaps.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    Resources.DebugTextures.EmplaceBack(
        LightSetup.DirLightShadowMapSRV,
        LightSetup.DirLightShadowMaps,
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        LightSetup.IrradianceMap.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        LightSetup.SpecularIrradianceMap.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        Resources.IntegrationLUT.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    if (GlobalEnableFXAA.GetBool())
    {
        PerformFXAA(CmdList);
    }
    else
    {
        PerformBackBufferBlit(CmdList);
    }

    ForwardRenderer.Render(
        CmdList,
        Resources,
        LightSetup);

    // Draw DebugBoxes
    if (GlobalDrawAABBs.GetBool())
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

        CmdList.SetShadingRate(EShadingRate::ShadingRate_1x1);

        DebugUI::Render(CmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End UI Render");

    // Finalize Commandlist
    CmdList.TransitionTexture(
        Resources.BackBuffer, 
        EResourceState::ResourceState_RenderTarget, 
        EResourceState::ResourceState_Present);
    
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--END FRAME--");

    CmdList.End();

    LastFrameNumDrawCalls     = CmdList.GetNumDrawCalls();
    LastFrameNumDispatchCalls = CmdList.GetNumDispatchCalls();
    LastFrameNumCommands      = CmdList.GetNumCommands();

    {
        TRACE_SCOPE("ExecuteCommandList");
        gCmdListExecutor.ExecuteCommandList(CmdList);
    }

    {
        TRACE_SCOPE("Present");
        Resources.MainWindowViewport->Present(GlobalVSyncEnabled.GetBool());
    }
}

Bool Renderer::Init()
{
    INIT_CONSOLE_VARIABLE("r.DrawTextureDebugger", GlobalDrawTextureDebugger);
    GlobalDrawTextureDebugger.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.DrawRendererInfo", GlobalDrawRendererInfo);
    GlobalDrawRendererInfo.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnableSSAO", GlobalEnableSSAO);
    GlobalEnableSSAO.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableFXAA", GlobalEnableFXAA);
    GlobalEnableFXAA.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnablePrePass", GlobalPrePassEnabled);
    GlobalPrePassEnabled.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableDrawAABBs", GlobalDrawAABBs);
    GlobalDrawAABBs.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnableVerticalSync", GlobalVSyncEnabled);
    GlobalVSyncEnabled.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.EnableFrustumCulling", GlobalFrustumCullEnabled);
    GlobalFrustumCullEnabled.SetBool(true);

    INIT_CONSOLE_VARIABLE("r.EnableRayTracing", GlobalRayTracingEnabled);
    GlobalRayTracingEnabled.SetBool(false);

    Resources.MainWindowViewport = RenderLayer::CreateViewport(
        gMainWindow,
        0, 0,
        EFormat::Format_R8G8B8A8_Unorm,
        EFormat::Format_Unknown);
    if (!Resources.MainWindowViewport)
    {
        return false;
    }
    else
    {
        Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    Resources.CameraBuffer = RenderLayer::CreateConstantBuffer<CameraBufferDesc>(
        nullptr, 
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
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
        { "POSITION", 0, EFormat::Format_R32G32B32_Float, 0, 0,  EInputClassification::InputClassification_Vertex, 0 },
        { "NORMAL",   0, EFormat::Format_R32G32B32_Float, 0, 12, EInputClassification::InputClassification_Vertex, 0 },
        { "TANGENT",  0, EFormat::Format_R32G32B32_Float, 0, 24, EInputClassification::InputClassification_Vertex, 0 },
        { "TEXCOORD", 0, EFormat::Format_R32G32_Float,    0, 36, EInputClassification::InputClassification_Vertex, 0 },
    };

    Resources.StdInputLayout = RenderLayer::CreateInputLayout(InputLayout);
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
        CreateInfo.AddressU = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressV = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressW = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipPoint;

        Resources.ShadowMapSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!Resources.ShadowMapSampler)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            Resources.ShadowMapSampler->SetName("ShadowMap Sampler");
        }
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU       = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressV       = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressW       = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.Filter         = ESamplerFilter::SamplerFilter_Comparison_MinMagMipLinear;
        CreateInfo.ComparisonFunc = EComparisonFunc::ComparisonFunc_LessEqual;

        Resources.ShadowMapCompSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!Resources.ShadowMapCompSampler)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            Resources.ShadowMapCompSampler->SetName("ShadowMap Comparison Sampler");
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

    CmdList.Begin();

    LightProbeRenderer.RenderSkyLightProbe(
        CmdList, 
        LightSetup, 
        Resources);

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    // TODO: Fix inital state of textures
    CmdList.Begin();

    CmdList.TransitionTexture(
        LightSetup.PointLightShadowMaps.Get(), 
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        LightSetup.DirLightShadowMaps.Get(),
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);
    
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

    Resources.Release();
    LightSetup.Release();

    RaytracingPSO.Reset();
    RayTracingScene.Reset();
    RayTracingGeometryInstances.Clear();

    AABBVertexBuffer.Reset();
    AABBIndexBuffer.Reset();
    AABBDebugPipelineState.Reset();

    PostPSO.Reset();
    FXAAPSO.Reset();

   LastFrameNumDrawCalls     = 0;
   LastFrameNumDispatchCalls = 0;
   LastFrameNumCommands      = 0;
}

Bool Renderer::InitBoundingBoxDebugPass()
{
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Debug.hlsl",
        "VSMain",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Debug VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Debug.hlsl",
        "PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("Debug PixelShader");
    }

    InputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::Format_R32G32B32_Float, 0, 0, EInputClassification::InputClassification_Vertex, 0 },
    };

    TSharedRef<InputLayoutState> InputLayoutState = RenderLayer::CreateInputLayout(InputLayout);
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
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_LessEqual;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_Zero;

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
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
    RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
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

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
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
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::PrimitiveTopologyType_Line;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = Resources.RenderTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = Resources.DepthBufferFormat;

    AABBDebugPipelineState = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!AABBDebugPipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBDebugPipelineState->SetName("Debug PipelineState");
    }

    XMFLOAT3 Vertices[8] =
    {
        { -0.5f, -0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },

        {  0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f }
    };

    ResourceData VertexData(Vertices);

    AABBVertexBuffer = RenderLayer::CreateVertexBuffer<XMFLOAT3>(&VertexData, 8, BufferUsage_Default);
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
    UInt16 Indices[24] =
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

    ResourceData IndexData(Indices);

    AABBIndexBuffer = RenderLayer::CreateIndexBuffer(
        &IndexData,
        sizeof(UInt16) * 24,
        EIndexFormat::IndexFormat_UInt16,
        BufferUsage_Default);
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
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/FullscreenVS.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Fullscreen VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/PostProcessPS.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("PostProcess PixelShader");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_Always;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_Zero;

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
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
    RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
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

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
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
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::Format_R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = EFormat::Format_Unknown;

    PostPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
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
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/FXAA_PS.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("FXAA PixelShader");
    }

    PSOProperties.ShaderState.PixelShader = PShader.Get();

    FXAAPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!FXAAPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FXAAPSO->SetName("FXAA PipelineState");
    }

    return true;
}

void Renderer::ResizeResources(UInt32 Width, UInt32 Height)
{
    gCmdListExecutor.WaitForGPU();

    Resources.MainWindowViewport->Resize(Width, Height);

    // TODO: Resize other resources
}
