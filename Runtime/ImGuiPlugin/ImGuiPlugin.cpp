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

// ImGui Forward declarations
static EWindowStyleFlags GetWindowStyleFromImGuiViewportFlags(ImGuiViewportFlags Flags);
static void              PlatformCreateWindow(ImGuiViewport* Viewport);
static void              PlatformDestroyWindow(ImGuiViewport* Viewport);
static void              PlatformShowWindow(ImGuiViewport* Viewport);
static void              PlatformUpdateWindow(ImGuiViewport* Viewport);
static ImVec2            PlatformGetWindowPos(ImGuiViewport* Viewport);
static void              PlatformSetWindowPosition(ImGuiViewport* Viewport, ImVec2 Position);
static ImVec2            PlatformGetWindowSize(ImGuiViewport* Viewport);
static void              PlatformSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
static void              PlatformSetWindowFocus(ImGuiViewport* Viewport);
static bool              PlatformGetWindowFocus(ImGuiViewport* Viewport);
static bool              PlatformGetWindowMinimized(ImGuiViewport* Viewport);
static void              PlatformSetWindowTitle(ImGuiViewport* Viewport, const CHAR* Title);
static void              PlatformSetWindowAlpha(ImGuiViewport* Viewport, float Alpha);
static float             PlatformGetWindowDpiScale(ImGuiViewport* Viewport);
static void              PlatformOnChangedViewport(ImGuiViewport*);

FImGuiPlugin* GImGuiPlugin = nullptr;

FImGuiPlugin::FImGuiPlugin()
    : IImguiPlugin()
    , PluginImGuiIO(nullptr)
    , PluginImGuiContext(nullptr)
    , Widgets()
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

        PlatformState.Platform_CreateWindow       = PlatformCreateWindow;
        PlatformState.Platform_DestroyWindow      = PlatformDestroyWindow;
        PlatformState.Platform_ShowWindow         = PlatformShowWindow;
        PlatformState.Platform_SetWindowPos       = PlatformSetWindowPosition;
        PlatformState.Platform_GetWindowPos       = PlatformGetWindowPos;
        PlatformState.Platform_SetWindowSize      = PlatformSetWindowSize;
        PlatformState.Platform_GetWindowSize      = PlatformGetWindowSize;
        PlatformState.Platform_SetWindowFocus     = PlatformSetWindowFocus;
        PlatformState.Platform_GetWindowFocus     = PlatformGetWindowFocus;
        PlatformState.Platform_GetWindowMinimized = PlatformGetWindowMinimized;
        PlatformState.Platform_SetWindowTitle     = PlatformSetWindowTitle;
        PlatformState.Platform_SetWindowAlpha     = PlatformSetWindowAlpha;
        PlatformState.Platform_UpdateWindow       = PlatformUpdateWindow;
        PlatformState.Platform_GetWindowDpiScale  = PlatformGetWindowDpiScale;
        PlatformState.Platform_OnChangedViewport  = PlatformOnChangedViewport;
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

    if (FWindowedApplication::IsInitialized())
    {
        EventHandler = MakeShared<FImGuiEventHandler>();
        FWindowedApplication::Get().RegisterInputHandler(EventHandler);
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
    if (FWindowedApplication::IsInitialized())
    {
        FWindowedApplication::Get().UnregisterInputHandler(EventHandler);
        EventHandler.Reset();
    }

    if (CVarImGuiEnableMultiViewports.GetValue())
    {
        ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
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
    TSharedPtr<FWindow> MainWindow = FWindowedApplication::Get().FindWindowWidget(MainViewport);
    if (!MainWindow)
    {
        return;
    }

    // Retrieve all windows necessary
    TSharedRef<FGenericWindow> PlatformWindow   = MainWindow->GetPlatformWindow();
    TSharedRef<FGenericWindow> ForegroundWindow = FWindowedApplication::Get().GetPlatformApplication()->GetForegroundWindow();

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.DeltaTime               = Delta / 1000.0f;
    UIState.DisplaySize             = ImVec2(static_cast<float>(MainWindow->GetWidth()), static_cast<float>(MainWindow->GetHeight()));
    UIState.FontGlobalScale         = CVarImGuiUseWindowDPIScale.GetValue() ? MainWindow->GetWindowDpiScale() : 1.0f;
    UIState.DisplayFramebufferScale = ImVec2(UIState.FontGlobalScale, UIState.FontGlobalScale);
    
    // Update Mouse
    ImGuiViewport* ForegroundViewport = ForegroundWindow ? ImGui::FindViewportByPlatformHandle(ForegroundWindow.Get()) : nullptr;

    const bool bIsTrackingMouse = false;
    const bool bIsAppFocused    = ForegroundWindow && (ForegroundWindow == PlatformWindow || PlatformWindow->IsChildWindow(ForegroundWindow) || ForegroundViewport);
    if (bIsAppFocused)
    {
        FWindowShape WindowShape;
        ForegroundWindow->GetWindowShape(WindowShape);

        if (UIState.WantSetMousePos)
        {
            ImVec2 MousePos = UIState.MousePos;
            if (!ImGuiExtensions::IsMultiViewportEnabled())
            {
                MousePos.x = MousePos.x - WindowShape.Position.x;
                MousePos.y = MousePos.y - WindowShape.Position.y;
            }

            FWindowedApplication::Get().SetCursorScreenPosition(FIntVector2(static_cast<int32>(MousePos.x), static_cast<int32>(MousePos.y)));
        }
        else /* if (!UIState.WantSetMousePos && !bIsTrackingMouse) */
        {
            FIntVector2 CursorPos = FWindowedApplication::Get().GetCursorScreenPosition();
            if (!ImGuiExtensions::IsMultiViewportEnabled())
            {
                CursorPos.x = CursorPos.x - WindowShape.Position.x;
                CursorPos.y = CursorPos.y - WindowShape.Position.y;
            }

            UIState.AddMousePosEvent(static_cast<float>(CursorPos.x), static_cast<float>(CursorPos.y));
        }
    }

    ImGuiID MouseViewportID = 0;
    if (TSharedRef<FGenericWindow> WindowUnderCursor = FWindowedApplication::Get().GetPlatformApplication()->GetWindowUnderCursor())
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
            FWindowedApplication::Get().SetCursor(ECursor::None);
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

            FWindowedApplication::Get().SetCursor(Cursor);
        }
    }

    UIState.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (FWindowedApplication::Get().IsGamePadConnected())
    {
        UIState.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }

    // Draw all ImGui widgets
    ImGui::NewFrame();

    Widgets.Foreach([](TSharedPtr<IImGuiWidget>& Widget)
    {
        Widget->Draw();
    });

    ImGui::EndFrame();
}

void FImGuiPlugin::Draw(FRHICommandList& CommandList)
{
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

void FImGuiPlugin::AddWidget(const TSharedPtr<IImGuiWidget>& InWidget)
{
    Widgets.AddUnique(InWidget);
}

void FImGuiPlugin::RemoveWidget(const TSharedPtr<IImGuiWidget>& InWidget)
{
    Widgets.Remove(InWidget);
}

void FImGuiPlugin::SetMainViewport(const TSharedPtr<FViewport>& InViewport)
{
    if (MainViewport == InViewport)
    {
        return;
    }

    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        TSharedPtr<IViewport> ViewportInterface = InViewport ? InViewport->GetViewportInterface() : nullptr;
        if (ViewportInterface)
        {
            Viewport->PlatformWindowCreated = true;
            Viewport->PlatformRequestMove   = true;
            Viewport->PlatformRequestResize = true;

            TSharedPtr<FWindow> MainWindow = FWindowedApplication::Get().FindWindowWidget(InViewport);
            Viewport->PlatformUserData  = MainWindow.Get();
            Viewport->PlatformHandle    = MainWindow->GetPlatformWindow().Get();
            Viewport->PlatformHandleRaw = MainWindow->GetPlatformWindow()->GetPlatformHandle();

            FImGuiViewport* ViewportData = new FImGuiViewport();
            ViewportData->Viewport      = ViewportInterface->GetViewportRHI();
            Viewport->RendererUserData  = ViewportData;
        }
        else
        {
            if (Viewport->RendererUserData)
            {
                FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->RendererUserData);
                Viewport->RendererUserData = nullptr;
                delete ViewportData;
            }

            Viewport->PlatformHandle        = nullptr;
            Viewport->PlatformHandleRaw     = nullptr;
            Viewport->PlatformUserData      = nullptr;
            Viewport->PlatformWindowCreated = false;
            Viewport->PlatformRequestMove   = false;
            Viewport->PlatformRequestResize = false;
        }
    }

    MainViewport = InViewport;
}


EWindowStyleFlags GetWindowStyleFromImGuiViewportFlags(ImGuiViewportFlags Flags)
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

void PlatformCreateWindow(ImGuiViewport* Viewport)
{
    const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);

    FGenericWindow* ParentWindow = nullptr;
    if (Viewport->ParentViewportId != 0)
    {
        if (ImGuiViewport* ParentViewport = ImGui::FindViewportByID(Viewport->ParentViewportId))
        {
            ParentWindow = reinterpret_cast<FGenericWindow*>(ParentViewport->PlatformUserData);
        }
    }

    FGenericWindowInitializer WindowInitializer;
    WindowInitializer.Title        = "Window";
    WindowInitializer.Width        = static_cast<uint32>(Viewport->Size.x);
    WindowInitializer.Height       = static_cast<uint32>(Viewport->Size.y);
    WindowInitializer.Position     = FIntVector2(static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
    WindowInitializer.Style        = WindowStyle;
    WindowInitializer.ParentWindow = ParentWindow;

    /* TSharedPtr<FWindow> Window = FApplication::Get().CreateWindow(WindowInitializer);
    if (Window)
    {
        Viewport->PlatformHandle        = Viewport->PlatformHandleRaw = Window->GetPlatformHandle();
        Viewport->PlatformUserData      = reinterpret_cast<void*>(Window.ReleaseOwnership());
        Viewport->PlatformRequestResize = false;
    }*/
}

void PlatformDestroyWindow(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        // if (Window == FApplication::Get().GetCapture())
        {
            // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
            // FApplication::Get().SetCapture(FApplication::Get().GetMainWindow());
        }

        Window->Destroy();
    }

    Viewport->PlatformUserData = Viewport->PlatformHandle = nullptr;
}

void PlatformShowWindow(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        if (Viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
        {
            Window->Show(false);
        }
        else
        {
            Window->Show();
        }
    }
}

void PlatformUpdateWindow(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);
        if (WindowStyle != Window->GetStyle())
        {
            Window->SetStyle(WindowStyle);

            const FWindowShape WindowShape(static_cast<uint32>(Viewport->Size.x), static_cast<uint32>(Viewport->Size.y), static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
            Window->SetWindowShape(WindowShape, false);

            if ((WindowStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None)
            {
                Window->SetWindowFocus();
            }

            Viewport->PlatformRequestMove = Viewport->PlatformRequestResize = true;
        }
    }
}

ImVec2 PlatformGetWindowPos(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        FWindowShape WindowShape;
        Window->GetWindowShape(WindowShape);
        return ImVec2(static_cast<float>(WindowShape.Position.x), static_cast<float>(WindowShape.Position.y));;
    }

    return ImVec2(0.0f, 0.0f);
}

void PlatformSetWindowPosition(ImGuiViewport* Viewport, ImVec2 Position)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        Window->SetWindowPos(static_cast<int32>(Position.x), static_cast<int32>(Position.y));
    }
}

ImVec2 PlatformGetWindowSize(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        FWindowShape WindowShape;
        Window->GetWindowShape(WindowShape);
        return ImVec2(static_cast<float>(WindowShape.Width), static_cast<float>(WindowShape.Height));;
    }

    return ImVec2(0.0f, 0.0f);
}

void PlatformSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        const FWindowShape WindowShape(static_cast<uint32>(Size.x), static_cast<uint32>(Size.y));
        Window->SetWindowShape(WindowShape, false);
    }
}

void PlatformSetWindowFocus(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        Window->SetWindowFocus();
    }
}

bool PlatformGetWindowFocus(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->IsActiveWindow();
    }

    return false;
}

bool PlatformGetWindowMinimized(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->IsMinimized();
    }

    return false;
}

void PlatformSetWindowTitle(ImGuiViewport* Viewport, const CHAR* Title)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->SetTitle(Title);
    }
}

void PlatformSetWindowAlpha(ImGuiViewport* Viewport, float Alpha)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->SetWindowOpacity(Alpha);
    }
}

float PlatformGetWindowDpiScale(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->GetWindowDpiScale();
    }

    return 1.0f;
}

void PlatformOnChangedViewport(ImGuiViewport*)
{
}