#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Platform/PlatformSystemClipboard.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Application/Application.h"
#include "Application/Widgets/ViewportWidget.h"
#include "RHI/RHICommandList.h"
#include "ImGuiPlugin/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include <imgui_internal.h>

IMPLEMENT_ENGINE_MODULE(FImGuiPlugin, ImGuiPlugin);

static TAutoConsoleVariable<bool> CVarImGuiUseWindowDPIScale(
    "ImGui.UseWindowDPIScale",
    "Scale ImGui elements with the Window DPI scale",
    false,
    EConsoleVariableFlags::Default);

#ifndef RELEASE_BUILD
static TAutoConsoleVariable<bool> CVarImGuiShowDemoWindow(
    "ImGui.ShowDemoWindow",
    "Show the ImGui Demo Window",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarImGuiEnableImGuiDelegates(
    "ImGui.EnableImGuiDelegates",
    "Enables drawing of registered ImGui delegates",
    true,
    EConsoleVariableFlags::Default);
#endif

static EWindowStyleFlags GetWindowStyleFromImGuiViewportFlags(ImGuiViewportFlags Flags)
{
    EWindowStyleFlags WindowStyleFlags = EWindowStyleFlags::None;
    if ((Flags & ImGuiViewportFlags_NoDecoration) == ImGuiViewportFlags_None)
    {
        WindowStyleFlags = EWindowStyleFlags::Titled | EWindowStyleFlags::Minimizable | EWindowStyleFlags::Maximizable | EWindowStyleFlags::Resizable | EWindowStyleFlags::Closable;
    }
    if (Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        WindowStyleFlags |= EWindowStyleFlags::NoTaskBarIcon;
    }
    if (Flags & ImGuiViewportFlags_TopMost)
    {
        WindowStyleFlags |= EWindowStyleFlags::TopMost;
    }

    return WindowStyleFlags;
}

static const char* GetImGuiViewportPlatformTitle(const ImGuiViewport* InViewport)
{
    if (!InViewport)
    {
        return "ImGui Window";
    }

    if (InViewport == ImGui::GetMainViewport())
    {
        return "ImGui Main Viewport";
    }

    const ImGuiViewportP* ViewportP = reinterpret_cast<const ImGuiViewportP*>(InViewport);
    if (ViewportP && ViewportP->Window && ViewportP->Window->Name && ViewportP->Window->Name[0] != '\0')
    {
        return ViewportP->Window->Name;
    }

    return "ImGui Window";
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
    , MonitorInfos()
    , DrawDelegates()
    , OnMonitorConfigChangedDelegateHandle()
    , bInputPassthroughEnabled(false)
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
        ImGuiIO& State = ImGui::GetIO();
        PluginImGuiIO = &State;
    }

    if (!PluginImGuiIO)
    {
        LOG_ERROR("Failed to create ImGuiContext");
        return false;
    }
    
    // Set the global-instance
    GImGuiPlugin = this;

    // Store this instance
    PluginImGuiIO->BackendPlatformUserData = this;
    
    // Setup flags
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasGamepad;              // Platform supports Gamepad and currently has one connected.
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    PluginImGuiIO->SetClipboardTextFn = [](void* UserData, const char* Text)
    {
        UNREFERENCED_VARIABLE(UserData);
        FPlatformSystemClipboard::SetText(String(Text));
    };

    PluginImGuiIO->GetClipboardTextFn = [](void* UserData) -> const char*
    {
        FImGuiPlugin* Context = reinterpret_cast<FImGuiPlugin*>(UserData);
        Context->ClipboardText.Clear();
        FPlatformSystemClipboard::GetText(Context->ClipboardText);
        return *Context->ClipboardText;
    };

    PluginImGuiIO->ClipboardUserData = this;

    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    
#ifdef EDITOR_BUILD
    // We have support for multiple Viewports, but we may not always want to utilize it
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
    
    // Configure viewports and docking in editor builds
    PluginImGuiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    PluginImGuiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

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

    PlatformState.Platform_CreateWindow = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnCreatePlatformWindow(Viewport);
    };
    
    PlatformState.Platform_DestroyWindow = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnDestroyPlatformWindow(Viewport);
    };

    PlatformState.Platform_ShowWindow = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnShowPlatformWindow(Viewport);
    };

    PlatformState.Platform_SetWindowPos = [](ImGuiViewport* Viewport, ImVec2 Position)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnSetPlatformWindowPosition(Viewport, Position);
    };

    PlatformState.Platform_GetWindowPos = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        return GImGuiPlugin->OnGetPlatformWindowPosition(Viewport);
    };

    PlatformState.Platform_SetWindowSize = [](ImGuiViewport* Viewport, ImVec2 Size)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnSetPlatformWindowSize(Viewport, Size);
    };

    PlatformState.Platform_GetWindowSize = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        return GImGuiPlugin->OnGetPlatformWindowSize(Viewport);
    };

    PlatformState.Platform_SetWindowFocus = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnSetPlatformWindowFocus(Viewport);
    };

    PlatformState.Platform_GetWindowFocus = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        return GImGuiPlugin->OnGetPlatformWindowFocus(Viewport);
    };

    PlatformState.Platform_GetWindowMinimized = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        return GImGuiPlugin->OnGetPlatformWindowMinimized(Viewport);
    };

    PlatformState.Platform_SetWindowTitle = [](ImGuiViewport* Viewport, const CHAR* Title)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnSetPlatformWindowTitle(Viewport, Title);
    };

    PlatformState.Platform_SetWindowAlpha = [](ImGuiViewport* Viewport, float Alpha)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnSetPlatformWindowAlpha(Viewport, Alpha);
    };

    PlatformState.Platform_UpdateWindow = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnUpdatePlatformWindow(Viewport);
    };

    PlatformState.Platform_GetWindowDpiScale = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        return GImGuiPlugin->OnGetPlatformWindowDpiScale(Viewport);
    };

    PlatformState.Platform_OnChangedViewport = [](ImGuiViewport* Viewport)
    {
        CHECK(GImGuiPlugin != nullptr);
        GImGuiPlugin->OnPlatformChangedViewport(Viewport);
    };
#else
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
#endif

    // Update monitor info
    UpdateMonitorInfo();

    // Default font
    InitializeDefaultFont();

    // Setup the style
    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    // Use AA for lines etc.
    Style.AntiAliasedLines = true;
    Style.AntiAliasedFill  = true;

    // New Style
    Style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    Style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    Style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_Border]                = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    Style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    Style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    Style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    Style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    Style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    Style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    Style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    Style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    Style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    Style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    Style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    Style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    Style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    Style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    Style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    Style.Colors[ImGuiCol_Button]                = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    Style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    Style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    Style.Colors[ImGuiCol_Header]                = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    Style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    Style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    Style.Colors[ImGuiCol_Separator]             = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    Style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    Style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    Style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    Style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    Style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    Style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    Style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    Style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    Style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    Style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    Style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    Style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    Style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    Style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    Style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    Style.PopupRounding = 3;

    Style.WindowPadding = ImVec2(4, 4);
    Style.FramePadding  = ImVec2(6, 4);
    Style.ItemSpacing   = ImVec2(6, 2);

    Style.ScrollbarSize = 18;

    Style.WindowBorderSize = 1;
    Style.ChildBorderSize  = 1;
    Style.PopupBorderSize  = 1;
    Style.FrameBorderSize  = 0;
    
    Style.WindowRounding    = 3;
    Style.ChildRounding     = 3;
    Style.FrameRounding     = 3;
    Style.ScrollbarRounding = 2;
    Style.GrabRounding      = 3;

#ifdef IMGUI_HAS_DOCK
    Style.TabBorderSize = 0;
    Style.TabRounding   = 3;

    Style.Colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    Style.Colors[ImGuiCol_Tab]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    Style.Colors[ImGuiCol_TabActive]          = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    Style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    Style.Colors[ImGuiCol_DockingPreview]     = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

    if (PluginImGuiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        Style.WindowRounding = 0.0f;
        Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif

    if (FApplication::IsInitialized())
    {
        EventHandler = MakeSharedPtr<FImGuiEventHandler>();
        FApplication::Get().RegisterInputHandler(EventHandler);

        OnMonitorConfigChangedDelegateHandle = FApplication::Get().GetOnMonitorConfigChangedEvent().AddRaw(this, &FImGuiPlugin::UpdateMonitorInfo);
    }
    else
    {
        LOG_ERROR("Appliation is not initialized, delay plugin loading");
        return false;
    }
    
    return true;
}

bool FImGuiPlugin::Unload()
{
    if (FApplication::IsInitialized())
    {
        FApplication::Get().UnregisterInputHandler(EventHandler);
        EventHandler.Reset();

        FApplication::Get().GetOnMonitorConfigChangedEvent().Unbind(OnMonitorConfigChangedDelegateHandle);
    }

    // Reset the backend pointer ...
    PluginImGuiIO->BackendPlatformUserData = nullptr;
    PluginImGuiIO->BackendRendererUserData = nullptr;

    // ... then destroy the context, this needs to happen in this order to avoid any asserts
    ImGui::DestroyContext(PluginImGuiContext);
    
    // Release the renderer here since the renderer could call functions within the DestroyContext function
    // if there is still an open window.
    ReleaseRHI();

    // Reset the global instance
    GImGuiPlugin = nullptr;

    // Reset cached pointers
    PluginImGuiIO      = nullptr;
    PluginImGuiContext = nullptr;
    return true;
}

bool FImGuiPlugin::InitializeRHI()
{
    Renderer = MakeSharedPtr<FImGuiRenderer>();
    if (!Renderer->InitializeRHI())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ImGuiRenderer");
        return false;
    }

    return true;
}

void FImGuiPlugin::ReleaseRHI()
{
    if (Renderer)
    {
        Renderer->ReleaseRHI();
        Renderer.Reset();
    }
}

bool FImGuiPlugin::UpdateFontAtlas()
{
    if (Renderer)
    {
        return Renderer->UpdateFontAtlas();
    }
    else
    {
        return false;
    }
}

void FImGuiPlugin::NewFrame(float DeltaTime)
{
    CHECK(MainWindow != nullptr);
    TSharedRef<IPlatformWindow> PlatformWindow = MainWindow->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    PluginImGuiIO->DeltaTime               = DeltaTime;
    PluginImGuiIO->DisplaySize             = ImVec2(static_cast<float>(MainWindow->GetWidth()), static_cast<float>(MainWindow->GetHeight()));
    PluginImGuiIO->FontGlobalScale         = CVarImGuiUseWindowDPIScale.GetValue() ? MainWindow->GetWindowDPIScale() : 1.0f;
    PluginImGuiIO->DisplayFramebufferScale = ImVec2(PluginImGuiIO->FontGlobalScale, PluginImGuiIO->FontGlobalScale);

    TSharedPtr<FWindowWidget>   ForegroundWindow = FApplication::Get().GetFocusWindow();
    TSharedRef<IPlatformWindow> PlatformForegroundWindow = ForegroundWindow ? ForegroundWindow->GetPlatformWindow() : nullptr;

    ImGuiViewport* ForegroundViewport = ForegroundWindow ? ImGui::FindViewportByPlatformHandle(ForegroundWindow.Get()) : nullptr;

    const bool bIsAppFocused = ForegroundWindow && (ForegroundWindow == MainWindow || PlatformWindow->IsChildWindow(PlatformForegroundWindow) || ForegroundViewport);
    if (bIsAppFocused && !bInputPassthroughEnabled)
    {
    #ifndef EDITOR_BUILD
        const IntVector2 ForegroundWindowPosition = ForegroundWindow->GetPosition();
    #endif

        const bool bIsTrackingMouse = FApplication::Get().IsTrackingCursor();
        if (PluginImGuiIO->WantSetMousePos)
        {
            ImVec2 MousePos = PluginImGuiIO->MousePos;
        #ifndef EDITOR_BUILD
            MousePos.x = MousePos.x - ForegroundWindowPosition.X;
            MousePos.y = MousePos.y - ForegroundWindowPosition.Y;
        #endif

            const IntVector2 CursorPos = IntVector2(static_cast<int32>(MousePos.x), static_cast<int32>(MousePos.y));
            FApplication::Get().SetCursorPosition(CursorPos);
        }
        else if (!bIsTrackingMouse)
        {
            IntVector2 CursorPos = FApplication::Get().GetCursorPosition();
        #ifndef EDITOR_BUILD
            CursorPos.X = CursorPos.X - ForegroundWindowPosition.X;
            CursorPos.Y = CursorPos.Y - ForegroundWindowPosition.Y;
        #endif

            PluginImGuiIO->AddMousePosEvent(static_cast<float>(CursorPos.X), static_cast<float>(CursorPos.Y));
        }
    }

    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    for (ImGuiViewport* PlatformViewport : PlatformState.Viewports)
    {
        if (FWindowWidget* ViewportWindow = reinterpret_cast<FWindowWidget*>(PlatformViewport->PlatformHandle))
        {
            ViewportWindow->SetAcceptsInput((PlatformViewport->Flags & ImGuiViewportFlags_NoInputs) == 0);
        }
    }

    ImGuiID MouseViewportID = 0;
    if (TSharedPtr<FWindowWidget> WindowUnderCursor = FApplication::Get().FindWindowUnderCursor())
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(WindowUnderCursor.Get()))
        {
            MouseViewportID = Viewport->ID;
        }
    }

    PluginImGuiIO->AddMouseViewportEvent(MouseViewportID);

    // Update the cursor type
    const bool bNoMouseCursorChange = (PluginImGuiIO->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != ImGuiConfigFlags_None;
    if (!bNoMouseCursorChange)
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if (ImguiCursor == ImGuiMouseCursor_None || PluginImGuiIO->MouseDrawCursor)
        {
            FApplication::Get().SetCursor(ECursor::None);
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

            FApplication::Get().SetCursor(Cursor);
        }
    }

    PluginImGuiIO->BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (FApplication::Get().IsGamePadConnected())
    {
        PluginImGuiIO->BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }
}

void FImGuiPlugin::Tick(float DeltaTime)
{
    // New Frame
    {
        TRACE_SCOPE("ImGui New Frame");
        NewFrame(DeltaTime);
    }

    // Draw all ImGui widgets
    {
        TRACE_SCOPE("ImGui Callbacks");

        // New frame
        ImGui::NewFrame();
        BeginFrameDelegates.Broadcast();

        // Draw
    #ifndef RELEASE_BUILD
        bool bShowDemoWindow = CVarImGuiShowDemoWindow.GetValue();
        if (bShowDemoWindow)
        {
            ImGui::ShowDemoWindow(&bShowDemoWindow);
            CVarImGuiShowDemoWindow->SetAsBool(bShowDemoWindow, EConsoleVariableFlags::SetByCode);
        }

        const bool bEnableImGuiDelegates = CVarImGuiEnableImGuiDelegates.GetValue();
        if (bEnableImGuiDelegates)
        {
            DrawDelegates.Broadcast();
        }
    #else
        DrawDelegates.Broadcast();
    #endif

        // End frame
        EndFrameDelegates.Broadcast();
        ImGui::EndFrame();
    }
}

void FImGuiPlugin::Draw(FRHICommandList& CommandList)
{
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

void FImGuiPlugin::DrawViewports(FRHICommandList& CommandList)
{
    if (Renderer)
    {
        Renderer->RenderPlatformWindows(CommandList);
    }
}

FDelegateHandle FImGuiPlugin::AddDrawDelegate(const FImGuiDelegate& Delegate)
{
    return DrawDelegates.Add(Delegate);
}

void FImGuiPlugin::RemoveDrawDelegate(FDelegateHandle DelegateHandle)
{
    DrawDelegates.Unbind(DelegateHandle);
}

FDelegateHandle FImGuiPlugin::AddBeginFrameDelegate(const FImGuiDelegate& Delegate)
{
    return BeginFrameDelegates.Add(Delegate);
}

void FImGuiPlugin::RemoveBeginFrameDelegate(FDelegateHandle DelegateHandle)
{
    BeginFrameDelegates.Unbind(DelegateHandle);
}

FDelegateHandle FImGuiPlugin::AddEndFrameDelegate(const FImGuiDelegate& Delegate)
{
    return EndFrameDelegates.Add(Delegate);
}

void FImGuiPlugin::RemoveEndFrameDelegate(FDelegateHandle DelegateHandle)
{
    EndFrameDelegates.Unbind(DelegateHandle);
}

void FImGuiPlugin::SetMainViewport(const TSharedPtr<FViewportWidget>& InViewport)
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
        ViewportData->Window    = FApplication::Get().FindWindowWidget(InViewport);
        ViewportData->SwapChain = ViewportInterface->GetRHISwapChain();
            
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

    MainWindow   = FApplication::Get().FindWindowWidget(InViewport);
    MainViewport = InViewport;
}

void FImGuiPlugin::UpdateMonitorInfo()
{
    FApplication::Get().GetDisplayInfo(MonitorInfos);

    // Rebound on every display change, so the list has to be emptied or it accumulates duplicates.
    ImGui::GetPlatformIO().Monitors.resize(0);

    for (const FMonitorInfo& MonitorInfo : MonitorInfos)
    {
        ImGuiPlatformMonitor ImGuiMonitor;
        ImGuiMonitor.MainPos  = ImVec2(static_cast<float>(MonitorInfo.MainPosition.X), static_cast<float>(MonitorInfo.MainPosition.Y));
        ImGuiMonitor.MainSize = ImVec2(static_cast<float>(MonitorInfo.MainSize.X), static_cast<float>(MonitorInfo.MainSize.Y));
        ImGuiMonitor.WorkPos  = ImVec2(static_cast<float>(MonitorInfo.WorkPosition.X), static_cast<float>(MonitorInfo.WorkPosition.Y));
        ImGuiMonitor.WorkSize = ImVec2(static_cast<float>(MonitorInfo.WorkSize.X), static_cast<float>(MonitorInfo.WorkSize.Y));
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

void FImGuiPlugin::InitializeDefaultFont()
{
    ImGuiIO& State = ImGui::GetIO();
    State.Fonts->Clear();
    State.Fonts->AddFontDefault();
}

void FImGuiPlugin::OnCreatePlatformWindow(ImGuiViewport* Viewport)
{
    CHECK(Viewport->PlatformUserData == nullptr);

    FImGuiViewport* ViewportData = new FImGuiViewport();
    Viewport->PlatformUserData = ViewportData;

    TSharedPtr<FWindowWidget> ParentWindow;
    if (Viewport->ParentViewportId != 0)
    {
        if (ImGuiViewport* ParentViewport = ImGui::FindViewportByID(Viewport->ParentViewportId))
        {
            FImGuiViewport* ParentViewportData = reinterpret_cast<FImGuiViewport*>(ParentViewport->PlatformUserData);
            ParentWindow = ParentViewportData->Window;
        }
    }

    const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);

    FWindowWidget::FInitializer WindowInitializer;
    WindowInitializer.Title           = GetImGuiViewportPlatformTitle(Viewport);
    WindowInitializer.Size            = IntVector2(static_cast<int32>(Viewport->Size.x), static_cast<int32>(Viewport->Size.y));
    WindowInitializer.Position        = IntVector2(static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
    WindowInitializer.StyleFlags      = WindowStyle;
    WindowInitializer.ParentWindow    = ParentWindow;
    WindowInitializer.bActivateOnShow = !(Viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing);
    WindowInitializer.bAcceptsInput   = !(Viewport->Flags & ImGuiViewportFlags_NoInputs);

    ViewportData->Window = CreateWidget<FWindowWidget>(WindowInitializer);
    CHECK(ViewportData->Window != nullptr);

    FApplication::Get().CreateWindow(ViewportData->Window);
   
    TSharedRef<IPlatformWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    Viewport->PlatformHandle        = ViewportData->Window.Get();
    Viewport->PlatformHandleRaw     = PlatformWindow->GetPlatformHandle();
    Viewport->PlatformRequestMove   = false;
    Viewport->PlatformRequestResize = false;
    Viewport->PlatformWindowCreated = true;

    ViewportData->Window->SetOnWindowResized(FOnWindowResized::CreateLambda([PlatformHandle = Viewport->PlatformHandle](const IntVector2&)
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
        {
            Viewport->PlatformRequestResize = true;
        }
    }));

    ViewportData->Window->SetOnWindowMoved(FOnWindowMoved::CreateLambda([PlatformHandle = Viewport->PlatformHandle](const IntVector2&)
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

void FImGuiPlugin::OnDestroyPlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    // Wait for the GPU to finish with the current frame before destroying the window
    FRHICommandListExecutor::Get().WaitForCommands();

    // Destroy the platform window
    FApplication::Get().DestroyWindow(ViewportData->Window);

    Viewport->PlatformUserData      = nullptr;
    Viewport->PlatformHandle        = nullptr;
    Viewport->PlatformHandleRaw     = nullptr;
    Viewport->PlatformWindowCreated = false;
    Viewport->PlatformRequestClose  = false;
    
    delete ViewportData;
}

void FImGuiPlugin::OnShowPlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    if (Viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
    {
        ViewportData->Window->Show(false);
    }
    else
    {
        ViewportData->Window->Show();
    }
}

void FImGuiPlugin::OnUpdatePlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    ViewportData->Window->SetAcceptsInput((Viewport->Flags & ImGuiViewportFlags_NoInputs) == 0);

    const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);
    if (WindowStyle != ViewportData->Window->GetStyle())
    {
        ViewportData->Window->SetStyle(WindowStyle);

        Viewport->PlatformRequestMove   = true;
        Viewport->PlatformRequestResize = true;
    }
}

ImVec2 FImGuiPlugin::OnGetPlatformWindowPosition(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    const IntVector2 Position = ViewportData->Window->GetPosition();
    return ImVec2(static_cast<float>(Position.X), static_cast<float>(Position.Y));
}

void FImGuiPlugin::OnSetPlatformWindowPosition(ImGuiViewport* Viewport, ImVec2 Position)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    
    ViewportData->Window->MoveTo(IntVector2(static_cast<int32>(Position.x), static_cast<int32>(Position.y)));
    Viewport->PlatformRequestMove = false;
}

ImVec2 FImGuiPlugin::OnGetPlatformWindowSize(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    const IntVector2 Size = ViewportData->Window->GetSize();
    return ImVec2(static_cast<float>(Size.X), static_cast<float>(Size.Y));
}

void FImGuiPlugin::OnSetPlatformWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    
    ViewportData->Window->Resize(IntVector2(static_cast<int32>(Size.x), static_cast<int32>(Size.y)));
    Viewport->PlatformRequestResize = false;
}

void FImGuiPlugin::OnSetPlatformWindowFocus(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    ViewportData->Window->SetFocus();
}

bool FImGuiPlugin::OnGetPlatformWindowFocus(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->IsActive();
}

bool FImGuiPlugin::OnGetPlatformWindowMinimized(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->IsMinimized();
}

void FImGuiPlugin::OnSetPlatformWindowTitle(ImGuiViewport* Viewport, const CHAR* Title)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    ViewportData->Window->SetTitle(Title);
}

void FImGuiPlugin::OnSetPlatformWindowAlpha(ImGuiViewport* Viewport, float Alpha)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    ViewportData->Window->SetOpacity(Alpha);
}

float FImGuiPlugin::OnGetPlatformWindowDpiScale(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    return ViewportData->Window->GetWindowDPIScale();
}

void FImGuiPlugin::OnPlatformChangedViewport(ImGuiViewport*)
{
}
