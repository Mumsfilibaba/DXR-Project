#include "ImGuiPlugin.h"
#include "ImGuiRenderer.h"
#include "ImGuiExtensions.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Application/Application.h"
#include "Application/Widgets/Viewport.h"
#include "RHI/RHICommandList.h"

#include <imgui_internal.h>

IMPLEMENT_ENGINE_MODULE(FImGuiPlugin, ImGuiPlugin);

static TAutoConsoleVariable<bool> CVarImGuiEnableMultiViewports(
    "ImGui.EnableMultiViewports",
    "Enable multiple Viewports in ImGui",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarImGuiUseWindowDPIScale(
    "ImGui.UseWindowDPIScale",
    "Scale ImGui elements with the Window DPI scale",
    false,
    EConsoleVariableFlags::Default);

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

    // Store this instance
    PluginImGuiIO->BackendPlatformUserData = this;
    
    // Setup flags
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
        
        PlatformState.Platform_CreateWindow       = &FImGuiPlugin::StaticOnCreatePlatformWindow;
        PlatformState.Platform_DestroyWindow      = &FImGuiPlugin::StaticOnDestroyPlatformWindow;
        PlatformState.Platform_ShowWindow         = &FImGuiPlugin::StaticOnShowPlatformWindow;
        PlatformState.Platform_SetWindowPos       = &FImGuiPlugin::StaticOnSetPlatformWindowPosition;
        PlatformState.Platform_GetWindowPos       = &FImGuiPlugin::StaticOnGetPlatformWindowPosition;
        PlatformState.Platform_SetWindowSize      = &FImGuiPlugin::StaticOnSetPlatformWindowSize;
        PlatformState.Platform_GetWindowSize      = &FImGuiPlugin::StaticOnGetPlatformWindowSize;
        PlatformState.Platform_SetWindowFocus     = &FImGuiPlugin::StaticOnSetPlatformWindowFocus;
        PlatformState.Platform_GetWindowFocus     = &FImGuiPlugin::StaticOnGetPlatformWindowFocus;
        PlatformState.Platform_GetWindowMinimized = &FImGuiPlugin::StaticOnGetPlatformWindowMinimized;
        PlatformState.Platform_SetWindowTitle     = &FImGuiPlugin::StaticOnSetPlatformWindowTitle;
        PlatformState.Platform_SetWindowAlpha     = &FImGuiPlugin::StaticOnSetPlatformWindowAlpha;
        PlatformState.Platform_UpdateWindow       = &FImGuiPlugin::StaticOnUpdatePlatformWindow;
        PlatformState.Platform_GetWindowDpiScale  = &FImGuiPlugin::StaticOnGetPlatformWindowDpiScale;
        PlatformState.Platform_OnChangedViewport  = &FImGuiPlugin::StaticOnPlatformChangedViewport;
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

    // New Style
    Style.Colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    Style.Colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    Style.Colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_Border]                 = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    Style.Colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    Style.Colors[ImGuiCol_FrameBg]                = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    Style.Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    Style.Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    Style.Colors[ImGuiCol_TitleBg]                = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    Style.Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    Style.Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    Style.Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    Style.Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    Style.Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    Style.Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    Style.Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    Style.Colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    Style.Colors[ImGuiCol_SliderGrab]             = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    Style.Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    Style.Colors[ImGuiCol_Button]                 = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    Style.Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    Style.Colors[ImGuiCol_ButtonActive]           = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    Style.Colors[ImGuiCol_Header]                 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    Style.Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    Style.Colors[ImGuiCol_HeaderActive]           = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    Style.Colors[ImGuiCol_Separator]              = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    Style.Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    Style.Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    Style.Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    Style.Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    Style.Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    Style.Colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    Style.Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    Style.Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    Style.Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    Style.Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    Style.Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    Style.Colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    Style.Colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    Style.Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    Style.Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

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

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        Style.WindowRounding = 0.0f;
        Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif
    
    // --- Old Style ---
#if 0
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
#endif
    
    // --- Old Style ---
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
        EventHandler = MakeSharedPtr<FImGuiEventHandler>();
        FApplicationInterface::Get().RegisterInputHandler(EventHandler);

        OnMonitorConfigChangedDelegateHandle = FApplicationInterface::Get().GetOnMonitorConfigChangedEvent().AddRaw(this, &FImGuiPlugin::UpdateMonitorInfo);
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
    if (FApplicationInterface::IsInitialized())
    {
        FApplicationInterface::Get().UnregisterInputHandler(EventHandler);
        EventHandler.Reset();

        FApplicationInterface::Get().GetOnMonitorConfigChangedEvent().Unbind(OnMonitorConfigChangedDelegateHandle);
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendPlatformUserData = nullptr;

    ImGui::DestroyContext(PluginImGuiContext);
    return true;
}


bool FImGuiPlugin::InitRenderer()
{
    Renderer = MakeSharedPtr<FImGuiRenderer>();
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
    
    ImGuiViewport* ForegroundViewport = ForegroundWindow ? ImGui::FindViewportByPlatformHandle(ForegroundWindow.Get()) : nullptr;
    const bool bIsAppFocused = ForegroundWindow && (ForegroundWindow == MainWindow || PlatformWindow->IsChildWindow(PlatformForegroundWindow) || ForegroundViewport);
    if (bIsAppFocused)
    {
        const FIntVector2 ForegroundWindowPosition = ForegroundWindow->GetCachedPosition();

        const bool bIsTrackingMouse = FApplicationInterface::Get().IsTrackingCursor();
        if (UIState.WantSetMousePos)
        {
            ImVec2 MousePos = UIState.MousePos;
            if (!ImGuiExtensions::IsMultiViewportEnabled())
            {
                MousePos.x = MousePos.x - ForegroundWindowPosition.x;
                MousePos.y = MousePos.y - ForegroundWindowPosition.y;
            }

            const FIntVector2 CursorPos = FIntVector2(static_cast<int32>(MousePos.x), static_cast<int32>(MousePos.y));
            FApplicationInterface::Get().SetCursorPosition(CursorPos);
        }
        else if (!bIsTrackingMouse)
        {
            FIntVector2 CursorPos = FApplicationInterface::Get().GetCursorPosition();
            if (!ImGuiExtensions::IsMultiViewportEnabled())
            {
                CursorPos.x = CursorPos.x - ForegroundWindowPosition.x;
                CursorPos.y = CursorPos.y - ForegroundWindowPosition.y;
            }

            UIState.AddMousePosEvent(static_cast<float>(CursorPos.x), static_cast<float>(CursorPos.y));
        }
    }

    ImGuiID MouseViewportID = 0;
    if (TSharedPtr<FWindow> WindowUnderCursor = FApplicationInterface::Get().FindWindowUnderCursor())
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
void FImGuiPlugin::OnCreatePlatformWindow(ImGuiViewport* Viewport)
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
    WindowInitializer.StyleFlags = WindowStyle;

    ViewportData->Window = CreateWidget<FWindow>(WindowInitializer);
    CHECK(ViewportData->Window != nullptr);

    FApplicationInterface::Get().CreateWindow(ViewportData->Window);
   
    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    Viewport->PlatformHandle        = ViewportData->Window.Get();
    Viewport->PlatformHandleRaw     = PlatformWindow->GetPlatformHandle();
    Viewport->PlatformRequestMove   = false;
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

void FImGuiPlugin::OnDestroyPlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    // Wait for the GPU to finish with the current frame before resizing
    GRHICommandExecutor.WaitForCommands();
    
    FApplicationInterface::Get().DestroyWindow(ViewportData->Window);

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

    const EWindowStyleFlags WindowStyle = GetWindowStyleFromImGuiViewportFlags(Viewport->Flags);
    if (WindowStyle != ViewportData->Window->GetStyle())
    {
        ViewportData->Window->SetStyle(WindowStyle);

        // TODO: This should be moved into FWindowsWindow::SetStyle
    #if PLATFORM_WINDOWS
        if ((WindowStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None)
        {
            PlatformWindow->SetWindowFocus();
        }
    
        const FWindowShape WindowShape(static_cast<uint32>(Viewport->Size.x), static_cast<uint32>(Viewport->Size.y), static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
        PlatformWindow->SetWindowShape(WindowShape, true);
    #endif
        
        Viewport->PlatformRequestMove   = true;
        Viewport->PlatformRequestResize = true;
    }
}

ImVec2 FImGuiPlugin::OnGetPlatformWindowPosition(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    const FIntVector2 Position = ViewportData->Window->GetCachedPosition();
    return ImVec2(static_cast<float>(Position.x), static_cast<float>(Position.y));
}

void FImGuiPlugin::OnSetPlatformWindowPosition(ImGuiViewport* Viewport, ImVec2 Position)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    
    ViewportData->Window->MoveTo(FIntVector2(static_cast<int32>(Position.x), static_cast<int32>(Position.y)));
    Viewport->PlatformRequestMove = false;
}

ImVec2 FImGuiPlugin::OnGetPlatformWindowSize(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    const FIntVector2 Size = ViewportData->Window->GetCachedSize();
    return ImVec2(static_cast<float>(Size.x), static_cast<float>(Size.y));
}

void FImGuiPlugin::OnSetPlatformWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    
    // Wait for the GPU to finish with the current frame before resizing
    GRHICommandExecutor.WaitForCommands();
    
    ViewportData->Window->Resize(FIntVector2(static_cast<int32>(Size.x), static_cast<int32>(Size.y)));
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
    return ViewportData->Window->GetWindowDpiScale();
}

void FImGuiPlugin::OnPlatformChangedViewport(ImGuiViewport*)
{
}

void FImGuiPlugin::StaticOnCreatePlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiPlugin::Get()->OnCreatePlatformWindow(Viewport);
}

void FImGuiPlugin::StaticOnDestroyPlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiPlugin::Get()->OnDestroyPlatformWindow(Viewport);
}

void FImGuiPlugin::StaticOnShowPlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiPlugin::Get()->OnShowPlatformWindow(Viewport);
}

void FImGuiPlugin::StaticOnUpdatePlatformWindow(ImGuiViewport* Viewport)
{
    FImGuiPlugin::Get()->OnUpdatePlatformWindow(Viewport);
}

ImVec2 FImGuiPlugin::StaticOnGetPlatformWindowPosition(ImGuiViewport* Viewport)
{
    return FImGuiPlugin::Get()->OnGetPlatformWindowPosition(Viewport);
}

void FImGuiPlugin::StaticOnSetPlatformWindowPosition(ImGuiViewport* Viewport, ImVec2 Position)
{
    FImGuiPlugin::Get()->OnSetPlatformWindowPosition(Viewport, Position);
}

ImVec2 FImGuiPlugin::StaticOnGetPlatformWindowSize(ImGuiViewport* Viewport)
{
    return FImGuiPlugin::Get()->OnGetPlatformWindowSize(Viewport);
}

void FImGuiPlugin::StaticOnSetPlatformWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    FImGuiPlugin::Get()->OnSetPlatformWindowSize(Viewport, Size);
}

void FImGuiPlugin::StaticOnSetPlatformWindowFocus(ImGuiViewport* Viewport)
{
    FImGuiPlugin::Get()->OnSetPlatformWindowFocus(Viewport);
}

bool FImGuiPlugin::StaticOnGetPlatformWindowFocus(ImGuiViewport* Viewport)
{
    return FImGuiPlugin::Get()->OnGetPlatformWindowFocus(Viewport);
}

bool FImGuiPlugin::StaticOnGetPlatformWindowMinimized(ImGuiViewport* Viewport)
{
    return FImGuiPlugin::Get()->OnGetPlatformWindowMinimized(Viewport);
}

void FImGuiPlugin::StaticOnSetPlatformWindowTitle(ImGuiViewport* Viewport, const CHAR* Title)
{
    FImGuiPlugin::Get()->OnSetPlatformWindowTitle(Viewport, Title);
}

void FImGuiPlugin::StaticOnSetPlatformWindowAlpha(ImGuiViewport* Viewport, float Alpha)
{
    FImGuiPlugin::Get()->OnSetPlatformWindowAlpha(Viewport, Alpha);
}

float FImGuiPlugin::StaticOnGetPlatformWindowDpiScale(ImGuiViewport* Viewport)
{
    return FImGuiPlugin::Get()->OnGetPlatformWindowDpiScale(Viewport);
}

void FImGuiPlugin::StaticOnPlatformChangedViewport(ImGuiViewport* Viewport)
{
    FImGuiPlugin::Get()->OnPlatformChangedViewport(Viewport);
}
