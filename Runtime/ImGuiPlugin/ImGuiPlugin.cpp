#include "ImGuiPlugin.h"
#include "ImGuiRenderer.h"
#include "ImGuiExtensions.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Application/Application.h"
#include "Application/Widgets/Viewport.h"
#include <imgui_internal.h>

IMPLEMENT_ENGINE_MODULE(FImGuiPlugin, ImGuiPlugin);

static TAutoConsoleVariable<bool> CVarImGuiEnableMultiViewports(
    "ImGui.EnableMultiViewports",
    "Enable multiple Viewports in ImGui",
    false);

static TAutoConsoleVariable<bool> CVarImGuiUseWindowDPIScale(
    "ImGui.UseWindowDPIScale",
    "Scale ImGui elements with the Window DPI scale",
    false);

static EWindowStyleFlags GetWindowStyleFromImGuiViewportFlags(ImGuiViewportFlags Flags)
{
    EWindowStyleFlags WindowStyleFlags = EWindowStyleFlags::None;
    if ((Flags & ImGuiViewportFlags_NoDecoration) == ImGuiViewportFlags_None)
    {
        WindowStyleFlags = EWindowStyleFlags::Titled | EWindowStyleFlags::Minimizable | EWindowStyleFlags::Maximizable | EWindowStyleFlags::Resizeable | EWindowStyleFlags::Closable;
    }
    if (Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        WindowStyleFlags |= EWindowStyleFlags::NoTaskBarIcon;
    }
    if (Flags & ImGuiViewportFlags_TopMost)
    {
        WindowStyleFlags |= EWindowStyleFlags::NoTaskBarIcon;
    }

    return WindowStyleFlags;
}

FImGuiPlugin* GImGuiPlugin = nullptr;

FImGuiPlugin::FImGuiPlugin()
    : IImguiPlugin()
    , PluginImGuiIO(nullptr)
    , PluginImGuiContext(nullptr)
    , Renderer(nullptr)
    , EventHandler(nullptr)
    , MainWindow(nullptr)
    , MainViewport(nullptr)
    , DrawDelegates()
    , OnMonitorConfigChangedDelegateHandle()
{
}

FImGuiPlugin::~FImGuiPlugin()
{
}

bool FImGuiPlugin::Load()
{
    IMGUI_CHECKVERSION();

    PluginImGuiContext = ImGui::CreateContext();
    if (!PluginImGuiContext)
    {
        LOG_ERROR("Failed to create ImGuiContext");
        return false;
    }
    else
    {
        ImGuiIO& GenericIO = ImGui::GetIO();
        PluginImGuiIO = &GenericIO;
    }

    if (!PluginImGuiIO)
    {
        LOG_ERROR("Failed to create ImGuiContext");
        return false;
    }

    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasGamepad;              // Platform supports Gamepad and currently has one connected.
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    if (CVarImGuiEnableMultiViewports.GetValue())
    {
        // We have support for multiple Viewports, but we may not always want to utilize it
        PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
        PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
    }

    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    if (CVarImGuiEnableMultiViewports.GetValue())
    {
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Viewport->PlatformHandle        = nullptr;
            Viewport->PlatformHandleRaw     = nullptr;
            Viewport->PlatformWindowCreated = false;
            Viewport->PlatformRequestMove   = false;
            Viewport->PlatformRequestResize = false;
            Viewport->PlatformUserData      = nullptr;
            Viewport->RendererUserData      = nullptr;
        }

        PlatformState.Platform_CreateWindow       = &FImGuiPlugin::StaticPlatformCreateWindow;
        PlatformState.Platform_DestroyWindow      = &FImGuiPlugin::StaticPlatformDestroyWindow;
        PlatformState.Platform_ShowWindow         = &FImGuiPlugin::StaticPlatformShowWindow;
        PlatformState.Platform_SetWindowPos       = &FImGuiPlugin::StaticPlatformSetWindowPosition;
        PlatformState.Platform_GetWindowPos       = &FImGuiPlugin::StaticPlatformGetWindowPos;
        PlatformState.Platform_SetWindowSize      = &FImGuiPlugin::StaticPlatformSetWindowSize;
        PlatformState.Platform_GetWindowSize      = &FImGuiPlugin::StaticPlatformGetWindowSize;
        PlatformState.Platform_SetWindowFocus     = &FImGuiPlugin::StaticPlatformSetWindowFocus;
        PlatformState.Platform_GetWindowFocus     = &FImGuiPlugin::StaticPlatformGetWindowFocus;
        PlatformState.Platform_GetWindowMinimized = &FImGuiPlugin::StaticPlatformGetWindowMinimized;
        PlatformState.Platform_SetWindowTitle     = &FImGuiPlugin::StaticPlatformSetWindowTitle;
        PlatformState.Platform_SetWindowAlpha     = &FImGuiPlugin::StaticPlatformSetWindowAlpha;
        PlatformState.Platform_UpdateWindow       = &FImGuiPlugin::StaticPlatformUpdateWindow;
        PlatformState.Platform_GetWindowDpiScale  = &FImGuiPlugin::StaticPlatformGetWindowDpiScale;
        PlatformState.Platform_OnChangedViewport  = &FImGuiPlugin::StaticPlatformOnChangedViewport;
    }
    else
    {
        PlatformState.Platform_CreateWindow       = nullptr;
        PlatformState.Platform_DestroyWindow      = nullptr;
        PlatformState.Platform_ShowWindow         = nullptr;
        PlatformState.Platform_SetWindowPos       = nullptr;
        PlatformState.Platform_GetWindowPos       = nullptr;
        PlatformState.Platform_SetWindowSize      = nullptr;
        PlatformState.Platform_GetWindowSize      = nullptr;
        PlatformState.Platform_SetWindowFocus     = nullptr;
        PlatformState.Platform_GetWindowFocus     = nullptr;
        PlatformState.Platform_GetWindowMinimized = nullptr;
        PlatformState.Platform_SetWindowTitle     = nullptr;
        PlatformState.Platform_SetWindowAlpha     = nullptr;
        PlatformState.Platform_UpdateWindow       = nullptr;
        PlatformState.Platform_GetWindowDpiScale  = nullptr;
        PlatformState.Platform_OnChangedViewport  = nullptr;
    }

    PluginImGuiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (ImGuiExtensions::IsMultiViewportEnabled())
    {
        PluginImGuiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    // Update monitor info
    UpdateMonitorInfo();

    // Setup the style
    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    // Use AA for lines etc.
    Style.AntiAliasedLines = true;
    Style.AntiAliasedFill  = true;

    Style.Colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

    // Headers
    Style.Colors[ImGuiCol_Header]        = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    Style.Colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    Style.Colors[ImGuiCol_HeaderActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Buttons
    Style.Colors[ImGuiCol_Button]        = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    Style.Colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    Style.Colors[ImGuiCol_ButtonActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    Style.Colors[ImGuiCol_FrameBg]        = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    Style.Colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    Style.Colors[ImGuiCol_FrameBgActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    Style.Colors[ImGuiCol_Tab]                = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    Style.Colors[ImGuiCol_TabHovered]         = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    Style.Colors[ImGuiCol_TabActive]          = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    Style.Colors[ImGuiCol_TabUnfocused]       = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    Style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    Style.Colors[ImGuiCol_TitleBg]          = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    Style.Colors[ImGuiCol_TitleBgActive]    = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    Style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

#if 0 
    // Padding
    Style.FramePadding = ImVec2(6.0f, 4.0f);

    // Size
    Style.WindowBorderSize = 0.0f;
    Style.FrameBorderSize  = 1.0f;
    Style.ChildBorderSize  = 1.0f;
    Style.PopupBorderSize  = 1.0f;
    Style.ScrollbarSize    = 10.0f;
    Style.GrabMinSize      = 20.0f;

    // Rounding
    Style.WindowRounding    = 4.0f;
    Style.FrameRounding     = 4.0f;
    Style.PopupRounding     = 4.0f;
    Style.GrabRounding      = 4.0f;
    Style.TabRounding       = 4.0f;
    Style.ScrollbarRounding = 6.0f;

    Style.Colors[ImGuiCol_WindowBg].x = 0.075f;
    Style.Colors[ImGuiCol_WindowBg].y = 0.075f;
    Style.Colors[ImGuiCol_WindowBg].z = 0.075f;
    Style.Colors[ImGuiCol_WindowBg].w = 0.925f;

    Style.Colors[ImGuiCol_Text].x = 0.95f;
    Style.Colors[ImGuiCol_Text].y = 0.95f;
    Style.Colors[ImGuiCol_Text].z = 0.95f;
    Style.Colors[ImGuiCol_Text].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogram].x = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].y = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].z = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogramHovered].x = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].y = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].z = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBg].x = 0.025f;
    Style.Colors[ImGuiCol_TitleBg].y = 0.025f;
    Style.Colors[ImGuiCol_TitleBg].z = 0.025f;
    Style.Colors[ImGuiCol_TitleBg].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBgActive].x = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].y = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].z = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBg].x = 0.1f;
    Style.Colors[ImGuiCol_FrameBg].y = 0.1f;
    Style.Colors[ImGuiCol_FrameBg].z = 0.1f;
    Style.Colors[ImGuiCol_FrameBg].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgHovered].x = 0.2f;
    Style.Colors[ImGuiCol_FrameBgHovered].y = 0.2f;
    Style.Colors[ImGuiCol_FrameBgHovered].z = 0.2f;
    Style.Colors[ImGuiCol_FrameBgHovered].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgActive].x = 0.15f;
    Style.Colors[ImGuiCol_FrameBgActive].y = 0.15f;
    Style.Colors[ImGuiCol_FrameBgActive].z = 0.15f;
    Style.Colors[ImGuiCol_FrameBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_Button].x = 0.4f;
    Style.Colors[ImGuiCol_Button].y = 0.4f;
    Style.Colors[ImGuiCol_Button].z = 0.4f;
    Style.Colors[ImGuiCol_Button].w = 1.0f;

    Style.Colors[ImGuiCol_ButtonHovered].x = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].y = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].z = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].w = 1.0f;

    Style.Colors[ImGuiCol_ButtonActive].x = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].y = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].z = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].w = 1.0f;

    Style.Colors[ImGuiCol_CheckMark].x = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].y = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].z = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].w = 1.0f;

    Style.Colors[ImGuiCol_SliderGrab].x = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].y = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].z = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].w = 1.0f;

    Style.Colors[ImGuiCol_SliderGrabActive].x = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].y = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].z = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGrip].x = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].y = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].z = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGripHovered].x = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].y = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].z = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGripActive].x = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].y = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].z = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].w = 1.0f;

    Style.Colors[ImGuiCol_Tab].x = 0.55f;
    Style.Colors[ImGuiCol_Tab].y = 0.55f;
    Style.Colors[ImGuiCol_Tab].z = 0.55f;
    Style.Colors[ImGuiCol_Tab].w = 1.0f;

    Style.Colors[ImGuiCol_TabHovered].x = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].y = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].z = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TabActive].x = 0.25f;
    Style.Colors[ImGuiCol_TabActive].y = 0.25f;
    Style.Colors[ImGuiCol_TabActive].z = 0.25f;
    Style.Colors[ImGuiCol_TabActive].w = 1.0f;
#endif

    if (FApplicationInterface::IsInitialized())
    {
        EventHandler = MakeShared<FImGuiEventHandler>();
        FApplicationInterface::Get().RegisterInputHandler(EventHandler);

        OnMonitorConfigChangedDelegateHandle = FApplicationInterface::Get().GetOnMonitorConfigChangedEvent().AddRaw(this, &FImGuiPlugin::UpdateMonitorInfo);
    }
    else
    {
        LOG_ERROR("Appliation is not initialized, delay plugin loading");
        return false;
    }

    GImGuiPlugin = this;
    return true;
}

bool FImGuiPlugin::Unload()
{
    if (FApplicationInterface::IsInitialized())
    {
        FApplicationInterface::Get().UnregisterInputHandler(EventHandler);
        EventHandler.Reset();

        FApplicationInterface::Get().GetOnMonitorConfigChangedEvent().Unbind(OnMonitorConfigChangedDelegateHandle);
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendPlatformUserData = nullptr;

    ImGui::DestroyContext(PluginImGuiContext);

    GImGuiPlugin = nullptr;
    return true;
}

bool FImGuiPlugin::InitRenderer()
{
    Renderer = MakeShared<FImGuiRenderer>();
    if (!Renderer->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ViewportRenderer ");
        return false;
    }

    return true;
}

void FImGuiPlugin::ReleaseRenderer()
{
    Renderer.Reset();
}

void FImGuiPlugin::Tick(float Delta)
{
    CHECK(MainWindow != nullptr);
    TSharedRef<FGenericWindow> PlatformWindow = MainWindow->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.DeltaTime               = Delta / 1000.0f;
    UIState.DisplaySize             = ImVec2(static_cast<float>(MainWindow->GetWidth()), static_cast<float>(MainWindow->GetHeight()));
    UIState.FontGlobalScale         = CVarImGuiUseWindowDPIScale.GetValue() ? MainWindow->GetWindowDpiScale() : 1.0f;
    UIState.DisplayFramebufferScale = ImVec2(UIState.FontGlobalScale, UIState.FontGlobalScale);

    TSharedPtr<FWindow>        ForegroundWindow         = FApplicationInterface::Get().GetFocusWindow();
    TSharedRef<FGenericWindow> PlatformForegroundWindow = ForegroundWindow ? ForegroundWindow->GetPlatformWindow() : nullptr;
    
    // const bool bIsTrackingMouse = false;
    ImGuiViewport* ForegroundViewport = ForegroundWindow ? ImGui::FindViewportByPlatformHandle(ForegroundWindow.Get()) : nullptr;
    const bool bIsAppFocused = ForegroundWindow && (ForegroundWindow == MainWindow || PlatformWindow->IsChildWindow(PlatformForegroundWindow) || ForegroundViewport);
    if (bIsAppFocused)
    {
        FWindowShape WindowShape;
        CHECK(PlatformForegroundWindow != nullptr);
        PlatformForegroundWindow->GetWindowShape(WindowShape);

        if (UIState.WantSetMousePos)
        {
            ImVec2 MousePos = UIState.MousePos;
            if (!ImGuiExtensions::IsMultiViewportEnabled())
            {
                MousePos.x = MousePos.x - WindowShape.Position.x;
                MousePos.y = MousePos.y - WindowShape.Position.y;
            }

            const FIntVector2 CursorPos = FIntVector2(static_cast<int32>(MousePos.x), static_cast<int32>(MousePos.y));
            FApplicationInterface::Get().SetCursorScreenPosition(CursorPos);
        }
        else /* if (!UIState.WantSetMousePos && !bIsTrackingMouse) */
        {
            FIntVector2 CursorPos = FApplicationInterface::Get().GetCursorScreenPosition();
            if (!ImGuiExtensions::IsMultiViewportEnabled())
            {
                CursorPos.x = CursorPos.x - WindowShape.Position.x;
                CursorPos.y = CursorPos.y - WindowShape.Position.y;
            }

            UIState.AddMousePosEvent(static_cast<float>(CursorPos.x), static_cast<float>(CursorPos.y));
        }
    }

    ImGuiID MouseViewportID = 0;
    TSharedPtr<FGenericApplication> PlatformApplication = FApplicationInterface::Get().GetPlatformApplication();
    if (TSharedPtr<FWindow> WindowUnderCursor = FApplicationInterface::Get().FindWindowFromGenericWindow(PlatformApplication->GetWindowUnderCursor()))
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(WindowUnderCursor.Get()))
        {
            MouseViewportID = Viewport->ID;
        }
    }

    UIState.AddMouseViewportEvent(MouseViewportID);

    // Update the cursor type
    const bool bNoMouseCursorChange = (UIState.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != ImGuiConfigFlags_None;
    if (!bNoMouseCursorChange)
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if (ImguiCursor == ImGuiMouseCursor_None || UIState.MouseDrawCursor)
        {
            FApplicationInterface::Get().SetCursor(ECursor::None);
        }
        else
        {
            ECursor Cursor = ECursor::Arrow;
            switch (ImguiCursor)
            {
            case ImGuiMouseCursor_Arrow:      Cursor = ECursor::Arrow;      break;
            case ImGuiMouseCursor_TextInput:  Cursor = ECursor::TextInput;  break;
            case ImGuiMouseCursor_ResizeAll:  Cursor = ECursor::ResizeAll;  break;
            case ImGuiMouseCursor_ResizeEW:   Cursor = ECursor::ResizeEW;   break;
            case ImGuiMouseCursor_ResizeNS:   Cursor = ECursor::ResizeNS;   break;
            case ImGuiMouseCursor_ResizeNESW: Cursor = ECursor::ResizeNESW; break;
            case ImGuiMouseCursor_ResizeNWSE: Cursor = ECursor::ResizeNWSE; break;
            case ImGuiMouseCursor_Hand:       Cursor = ECursor::Hand;       break;
            case ImGuiMouseCursor_NotAllowed: Cursor = ECursor::NotAllowed; break;
            }

            FApplicationInterface::Get().SetCursor(Cursor);
        }
    }

    UIState.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (FApplicationInterface::Get().IsGamePadConnected())
    {
        UIState.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }

    // Draw all ImGui widgets
    ImGui::NewFrame();

    DrawDelegates.Broadcast();

    ImGui::EndFrame();
}

void FImGuiPlugin::Draw(FRHICommandList& CommandList)
{
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

FDelegateHandle FImGuiPlugin::AddDelegate(const FImGuiDelegate& Delegate)
{
    return DrawDelegates.Add(Delegate);
}

void FImGuiPlugin::RemoveDelegate(FDelegateHandle DelegateHandle)
{
    DrawDelegates.Unbind(DelegateHandle);
}

void FImGuiPlugin::SetMainViewport(const TSharedPtr<FViewport>& InViewport)
{
    if (MainViewport == InViewport)
    {
        return;
    }

    ImGuiViewport* Viewport = ImGui::GetMainViewport();
    CHECK(Viewport != nullptr);

    TSharedPtr<IViewport> ViewportInterface = InViewport ? InViewport->GetViewportInterface() : nullptr;
    if (ViewportInterface)
    {
        Viewport->PlatformWindowCreated = true;
        Viewport->PlatformRequestMove   = true;
        Viewport->PlatformRequestResize = true;

        FImGuiViewport* ViewportData = new FImGuiViewport();
        ViewportData->Window   = FApplicationInterface::Get().FindWindowWidget(InViewport);
        ViewportData->Viewport = ViewportInterface->GetViewportRHI();
            
        Viewport->PlatformHandle    = ViewportData->Window.Get();
        Viewport->PlatformHandleRaw = ViewportData->Window->GetPlatformWindow()->GetPlatformHandle();
        Viewport->PlatformUserData  = ViewportData;
        Viewport->RendererUserData  = ViewportData;
    }
    else
    {
        if (Viewport->RendererUserData)
        {
            FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
            delete ViewportData;
        }

        Viewport->RendererUserData      = nullptr;
        Viewport->PlatformHandle        = nullptr;
        Viewport->PlatformHandleRaw     = nullptr;
        Viewport->PlatformUserData      = nullptr;
        Viewport->PlatformWindowCreated = false;
        Viewport->PlatformRequestMove   = false;
        Viewport->PlatformRequestResize = false;
    }

    MainWindow   = FApplicationInterface::Get().FindWindowWidget(InViewport);
    MainViewport = InViewport;
}

void FImGuiPlugin::UpdateMonitorInfo()
{
    FDisplayInfo DisplayInfo;
    FApplicationInterface::Get().GetDisplayInfo(DisplayInfo);

    for (FMonitorInfo& MonitorInfo : DisplayInfo.MonitorInfos)
    {
        ImGuiPlatformMonitor ImGuiMonitor;
        ImGuiMonitor.MainPos  = ImVec2(static_cast<float>(MonitorInfo.MainPosition.x), static_cast<float>(MonitorInfo.MainPosition.y));
        ImGuiMonitor.MainSize = ImVec2(static_cast<float>(MonitorInfo.MainSize.x), static_cast<float>(MonitorInfo.MainSize.y));
        ImGuiMonitor.WorkPos  = ImVec2(static_cast<float>(MonitorInfo.WorkPosition.x), static_cast<float>(MonitorInfo.WorkPosition.y));
        ImGuiMonitor.WorkSize = ImVec2(static_cast<float>(MonitorInfo.WorkSize.x), static_cast<float>(MonitorInfo.WorkSize.y));
        ImGuiMonitor.DpiScale = MonitorInfo.DisplayScaling;

        ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
        if (MonitorInfo.bIsPrimary)
        {
            PlatformState.Monitors.push_front(ImGuiMonitor);
        }
        else
        {
            PlatformState.Monitors.push_back(ImGuiMonitor);
        }
    }
}

void FImGuiPlugin::StaticPlatformCreateWindow(ImGuiViewport* Viewport)
{
    CHECK(Viewport->PlatformUserData == nullptr);

    FImGuiViewport* ViewportData = new FImGuiViewport();
    Viewport->PlatformUserData = ViewportData;

    TSharedPtr<FWindow> ParentWindow;
    if (Viewport->ParentViewportId != 0)
    {
        if (ImGuiViewport* ParentViewport = ImGui::FindViewportByID(Viewport->ParentViewportId))
        {
            FImGuiViewport* ParentViewportData = reinterpret_cast<FImGuiViewport*>(ParentViewport->PlatformUserData);
            ParentWindow = ParentViewportData->Window;
        }
    }

    const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);

    FWindow::FInitializer WindowInitializer;
    WindowInitializer.Title      = "ImGui Window";
    WindowInitializer.Size       = FIntVector2(static_cast<int32>(Viewport->Size.x), static_cast<int32>(Viewport->Size.y));
    WindowInitializer.Position   = FIntVector2(static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
    WindowInitializer.WindowMode = EWindowMode::Windowed;

    ViewportData->Window = CreateWidget<FWindow>(WindowInitializer);
    CHECK(ViewportData->Window != nullptr);

    FApplicationInterface::Get().InitializeWindow(ViewportData->Window);

    Viewport->PlatformHandle        = ViewportData->Window.Get();
    Viewport->PlatformHandleRaw     = ViewportData->Window->GetPlatformWindow()->GetPlatformHandle();
    Viewport->PlatformRequestResize = false;
    Viewport->PlatformWindowCreated = true;

    ViewportData->Window->SetOnWindowResized(FOnWindowResized::CreateLambda([PlatformHandle = Viewport->PlatformHandle](const FIntVector2&)
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
        {
            Viewport->PlatformRequestResize = true;
        }
    }));

    ViewportData->Window->SetOnWindowMoved(FOnWindowMoved::CreateLambda([PlatformHandle = Viewport->PlatformHandle](const FIntVector2&)
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
        {
            Viewport->PlatformRequestMove = true;
        }
    }));

    ViewportData->Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([PlatformHandle = Viewport->PlatformHandle]()
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
        {
            Viewport->PlatformRequestClose = true;
        }
    }));
}

void FImGuiPlugin::StaticPlatformDestroyWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    FApplicationInterface::Get().DestroyWindow(ViewportData->Window);

    Viewport->PlatformUserData      = nullptr;
    Viewport->PlatformHandle        = nullptr;
    Viewport->PlatformHandleRaw     = nullptr;
    Viewport->PlatformWindowCreated = false;
    delete ViewportData;
}

void FImGuiPlugin::StaticPlatformShowWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    if (Viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
    {
        PlatformWindow->Show(false);
    }
    else
    {
        PlatformWindow->Show();
    }
}

void FImGuiPlugin::StaticPlatformUpdateWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);
    if (WindowStyle != PlatformWindow->GetStyle())
    {
        PlatformWindow->SetStyle(WindowStyle);

        if ((WindowStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None)
        {
            PlatformWindow->SetWindowFocus();
        }
    }

    const FWindowShape WindowShape(static_cast<uint32>(Viewport->Size.x), static_cast<uint32>(Viewport->Size.y), static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
    PlatformWindow->SetWindowShape(WindowShape, false);

    Viewport->PlatformRequestMove   = true;
    Viewport->PlatformRequestResize = true;
}

ImVec2 FImGuiPlugin::StaticPlatformGetWindowPos(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    FWindowShape WindowShape;
    PlatformWindow->GetWindowShape(WindowShape);
    return ImVec2(static_cast<float>(WindowShape.Position.x), static_cast<float>(WindowShape.Position.y));
}

void FImGuiPlugin::StaticPlatformSetWindowPosition(ImGuiViewport* Viewport, ImVec2 Position)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    PlatformWindow->SetWindowPos(static_cast<int32>(Position.x), static_cast<int32>(Position.y));
}

ImVec2 FImGuiPlugin::StaticPlatformGetWindowSize(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    FWindowShape WindowShape;
    PlatformWindow->GetWindowShape(WindowShape);
    return ImVec2(static_cast<float>(WindowShape.Width), static_cast<float>(WindowShape.Height));
}

void FImGuiPlugin::StaticPlatformSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    const FWindowShape WindowShape(static_cast<uint32>(Size.x), static_cast<uint32>(Size.y));
    PlatformWindow->SetWindowShape(WindowShape, false);
}

void FImGuiPlugin::StaticPlatformSetWindowFocus(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    PlatformWindow->SetWindowFocus();
}

bool FImGuiPlugin::StaticPlatformGetWindowFocus(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->IsActive();
}

bool FImGuiPlugin::StaticPlatformGetWindowMinimized(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->IsMinimized();
}

void FImGuiPlugin::StaticPlatformSetWindowTitle(ImGuiViewport* Viewport, const CHAR* Title)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->SetTitle(Title);
}

void FImGuiPlugin::StaticPlatformSetWindowAlpha(ImGuiViewport* Viewport, float Alpha)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    ViewportData->Window->SetWindowOpacity(Alpha);
}

float FImGuiPlugin::StaticPlatformGetWindowDpiScale(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->GetWindowDpiScale();
}

void FImGuiPlugin::StaticPlatformOnChangedViewport(ImGuiViewport*)
{
}