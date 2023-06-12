#include "Application.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Application);

static uint32 ImGui_GetMouseButtonIndex(EMouseButton Button)
{
    switch (Button)
    {
        case MouseButton_Left:    return ImGuiMouseButton_Left;
        case MouseButton_Right:   return ImGuiMouseButton_Right;
        case MouseButton_Middle:  return ImGuiMouseButton_Middle;
        case MouseButton_Back:    return 3;
        case MouseButton_Forward: return 4;
        default:                  return ImGuiKey_None;
    }
}

static ImGuiKey ImGui_KeyToImGuiKey(EKey Key)
{
    switch (Key)
    {
    case Key_Tab:            return ImGuiKey_Tab;
    case Key_Left:           return ImGuiKey_LeftArrow;
    case Key_Right:          return ImGuiKey_RightArrow;
    case Key_Up:             return ImGuiKey_UpArrow;
    case Key_Down:           return ImGuiKey_DownArrow;
    case Key_PageUp:         return ImGuiKey_PageUp;
    case Key_PageDown:       return ImGuiKey_PageDown;
    case Key_Home:           return ImGuiKey_Home;
    case Key_End:            return ImGuiKey_End;
    case Key_Insert:         return ImGuiKey_Insert;
    case Key_Delete:         return ImGuiKey_Delete;
    case Key_Backspace:      return ImGuiKey_Backspace;
    case Key_Space:          return ImGuiKey_Space;
    case Key_Enter:          return ImGuiKey_Enter;
    case Key_Escape:         return ImGuiKey_Escape;
    case Key_Apostrophe:     return ImGuiKey_Apostrophe;
    case Key_Comma:          return ImGuiKey_Comma;
    case Key_Minus:          return ImGuiKey_Minus;
    case Key_Period:         return ImGuiKey_Period;
    case Key_Slash:          return ImGuiKey_Slash;
    case Key_Semicolon:      return ImGuiKey_Semicolon;
    case Key_Equal:          return ImGuiKey_Equal;
    case Key_LeftBracket:    return ImGuiKey_LeftBracket;
    case Key_Backslash:      return ImGuiKey_Backslash;
    case Key_RightBracket:   return ImGuiKey_RightBracket;
    case Key_GraveAccent:    return ImGuiKey_GraveAccent;
    case Key_CapsLock:       return ImGuiKey_CapsLock;
    case Key_ScrollLock:     return ImGuiKey_ScrollLock;
    case Key_NumLock:        return ImGuiKey_NumLock;
    case Key_PrintScreen:    return ImGuiKey_PrintScreen;
    case Key_Pause:          return ImGuiKey_Pause;
    case Key_Keypad0:        return ImGuiKey_Keypad0;
    case Key_Keypad1:        return ImGuiKey_Keypad1;
    case Key_Keypad2:        return ImGuiKey_Keypad2;
    case Key_Keypad3:        return ImGuiKey_Keypad3;
    case Key_Keypad4:        return ImGuiKey_Keypad4;
    case Key_Keypad5:        return ImGuiKey_Keypad5;
    case Key_Keypad6:        return ImGuiKey_Keypad6;
    case Key_Keypad7:        return ImGuiKey_Keypad7;
    case Key_Keypad8:        return ImGuiKey_Keypad8;
    case Key_Keypad9:        return ImGuiKey_Keypad9;
    case Key_KeypadDecimal:  return ImGuiKey_KeypadDecimal;
    case Key_KeypadDivide:   return ImGuiKey_KeypadDivide;
    case Key_KeypadMultiply: return ImGuiKey_KeypadMultiply;
    case Key_KeypadSubtract: return ImGuiKey_KeypadSubtract;
    case Key_KeypadAdd:      return ImGuiKey_KeypadAdd;
    case Key_KeypadEnter:    return ImGuiKey_KeypadEnter;
    case Key_LeftShift:      return ImGuiKey_LeftShift;
    case Key_LeftControl:    return ImGuiKey_LeftCtrl;
    case Key_LeftAlt:        return ImGuiKey_LeftAlt;
    case Key_LeftSuper:      return ImGuiKey_LeftSuper;
    case Key_RightShift:     return ImGuiKey_RightShift;
    case Key_RightControl:   return ImGuiKey_RightCtrl;
    case Key_RightAlt:       return ImGuiKey_RightAlt;
    case Key_RightSuper:     return ImGuiKey_RightSuper;
    case Key_Menu:           return ImGuiKey_Menu;
    case Key_0:              return ImGuiKey_0;
    case Key_1:              return ImGuiKey_1;
    case Key_2:              return ImGuiKey_2;
    case Key_3:              return ImGuiKey_3;
    case Key_4:              return ImGuiKey_4;
    case Key_5:              return ImGuiKey_5;
    case Key_6:              return ImGuiKey_6;
    case Key_7:              return ImGuiKey_7;
    case Key_8:              return ImGuiKey_8;
    case Key_9:              return ImGuiKey_9;
    case Key_A:              return ImGuiKey_A;
    case Key_B:              return ImGuiKey_B;
    case Key_C:              return ImGuiKey_C;
    case Key_D:              return ImGuiKey_D;
    case Key_E:              return ImGuiKey_E;
    case Key_F:              return ImGuiKey_F;
    case Key_G:              return ImGuiKey_G;
    case Key_H:              return ImGuiKey_H;
    case Key_I:              return ImGuiKey_I;
    case Key_J:              return ImGuiKey_J;
    case Key_K:              return ImGuiKey_K;
    case Key_L:              return ImGuiKey_L;
    case Key_M:              return ImGuiKey_M;
    case Key_N:              return ImGuiKey_N;
    case Key_O:              return ImGuiKey_O;
    case Key_P:              return ImGuiKey_P;
    case Key_Q:              return ImGuiKey_Q;
    case Key_R:              return ImGuiKey_R;
    case Key_S:              return ImGuiKey_S;
    case Key_T:              return ImGuiKey_T;
    case Key_U:              return ImGuiKey_U;
    case Key_V:              return ImGuiKey_V;
    case Key_W:              return ImGuiKey_W;
    case Key_X:              return ImGuiKey_X;
    case Key_Y:              return ImGuiKey_Y;
    case Key_Z:              return ImGuiKey_Z;
    case Key_F1:             return ImGuiKey_F1;
    case Key_F2:             return ImGuiKey_F2;
    case Key_F3:             return ImGuiKey_F3;
    case Key_F4:             return ImGuiKey_F4;
    case Key_F5:             return ImGuiKey_F5;
    case Key_F6:             return ImGuiKey_F6;
    case Key_F7:             return ImGuiKey_F7;
    case Key_F8:             return ImGuiKey_F8;
    case Key_F9:             return ImGuiKey_F9;
    case Key_F10:            return ImGuiKey_F10;
    case Key_F11:            return ImGuiKey_F11;
    case Key_F12:            return ImGuiKey_F12;
    default:                 return ImGuiKey_None;
    }
}

static ImGuiKey ImGui_GetGamepadButton(EControllerButton Button)
{
    switch (Button)
    {
    case EControllerButton::Start:         return ImGuiKey_GamepadStart;
    case EControllerButton::Back:          return ImGuiKey_GamepadBack;
    case EControllerButton::DPadUp:        return ImGuiKey_GamepadDpadUp;
    case EControllerButton::DPadDown:      return ImGuiKey_GamepadDpadDown;
    case EControllerButton::DPadLeft:      return ImGuiKey_GamepadDpadLeft;
    case EControllerButton::DPadRight:     return ImGuiKey_GamepadDpadRight;
    case EControllerButton::FaceUp:        return ImGuiKey_GamepadFaceUp;
    case EControllerButton::FaceDown:      return ImGuiKey_GamepadFaceDown;
    case EControllerButton::FaceLeft:      return ImGuiKey_GamepadFaceLeft;
    case EControllerButton::FaceRight:     return ImGuiKey_GamepadFaceRight;
    case EControllerButton::RightTrigger:  return ImGuiKey_GamepadR3;
    case EControllerButton::LeftTrigger:   return ImGuiKey_GamepadL3;
    case EControllerButton::RightShoulder: return ImGuiKey_GamepadR3;
    case EControllerButton::LeftShoulder:  return ImGuiKey_GamepadL3;
    default:                               return ImGuiKey_None;
    }
}

static ImGuiKey ImGui_GetGamepadAnalog(EControllerAnalog Analog, bool bIsNegative)
{
    switch (Analog)
    {
    case EControllerAnalog::RightThumbX:  return bIsNegative ? ImGuiKey_GamepadRStickDown : ImGuiKey_GamepadRStickUp;
    case EControllerAnalog::RightThumbY:  return bIsNegative ? ImGuiKey_GamepadRStickLeft : ImGuiKey_GamepadRStickRight;
    case EControllerAnalog::LeftThumbX:   return bIsNegative ? ImGuiKey_GamepadLStickDown : ImGuiKey_GamepadLStickUp;
    case EControllerAnalog::LeftThumbY:   return bIsNegative ? ImGuiKey_GamepadLStickLeft : ImGuiKey_GamepadLStickRight;
    case EControllerAnalog::RightTrigger: return ImGuiKey_GamepadR2;
    case EControllerAnalog::LeftTrigger:  return ImGuiKey_GamepadL2;
    default:                              return ImGuiKey_None;
    }
}

static FWindowStyle ImGuiGetWindowStyleFromViewportFlags(ImGuiViewportFlags Flags)
{
    EWindowStyleFlag WindowStyleFlags = EWindowStyleFlag::None;
    if ((Flags & ImGuiViewportFlags_NoDecoration) == 0)
    {
        WindowStyleFlags = EWindowStyleFlag::Titled | EWindowStyleFlag::Minimizable | EWindowStyleFlag::Maximizable | EWindowStyleFlag::Resizeable | EWindowStyleFlag::Closable;
    }

    if (Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        WindowStyleFlags |= EWindowStyleFlag::NoTaskBarIcon;
    }

    if (Flags & ImGuiViewportFlags_TopMost)
    {
        WindowStyleFlags |= EWindowStyleFlag::NoTaskBarIcon;
    }

    return FWindowStyle(WindowStyleFlags);
}

static void ImGuiCreateWindow(ImGuiViewport* Viewport)
{
    const FWindowStyle WindowStyle = ImGuiGetWindowStyleFromViewportFlags(Viewport->Flags);

    FGenericWindow* ParentWindow = nullptr;
    if (Viewport->ParentViewportId != 0)
    {
        if (ImGuiViewport* ParentViewport = ImGui::FindViewportByID(Viewport->ParentViewportId))
        {
            ParentWindow = reinterpret_cast<FGenericWindow*>(ParentViewport->PlatformHandle);
        }
    }

    TSharedRef<FGenericWindow> Window = FWindowedApplication::Get().CreateWindow(FWindowInitializer()
        .SetTitle("Window")
        .SetWidth(static_cast<uint32>(Viewport->Size.x))
        .SetHeight(static_cast<uint32>(Viewport->Size.y))
        .SetPosition(FIntVector2(static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y)))
        .SetStyle(WindowStyle)
        .SetParentWindow(ParentWindow));

    if (Window)
    {
        Viewport->PlatformHandle   = Viewport->PlatformHandleRaw = Window->GetPlatformHandle();
        Viewport->PlatformUserData = reinterpret_cast<void*>(Window.ReleaseOwnership());
        Viewport->PlatformRequestResize = false;
    }
}

static void ImGuiDestroyWindow(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        if (Window == FWindowedApplication::Get().GetCapture())
        {
            // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
            FWindowedApplication::Get().SetCapture(FWindowedApplication::Get().GetMainWindow());
        }

        Window->Destroy();
    }

    Viewport->PlatformUserData = Viewport->PlatformHandle = nullptr;
}

static void ImGuiShowWindow(ImGuiViewport* Viewport)
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

static void ImGuiUpdateWindow(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        const FWindowStyle WindowStyle = ImGuiGetWindowStyleFromViewportFlags(Viewport->Flags);
        if (WindowStyle != Window->GetStyle())
        {
            Window->SetStyle(WindowStyle);

            const FWindowShape WindowShape(static_cast<uint32>(Viewport->Size.x), static_cast<uint32>(Viewport->Size.y), static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y));
            Window->SetWindowShape(WindowShape, false);

            if (WindowStyle.IsTopMost())
            {
                Window->SetWindowFocus();
            }

            Viewport->PlatformRequestMove = Viewport->PlatformRequestResize = true;
        }
    }
}

static ImVec2 ImGuiGetWindowPos(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        FWindowShape WindowShape;
        Window->GetWindowShape(WindowShape);

        const ImVec2 Result = ImVec2(static_cast<float>(WindowShape.Position.x), static_cast<float>(WindowShape.Position.y));
        return Result;
    }

    return ImVec2(0.0f, 0.0f);
}

static void ImGuiSetWindowPos(ImGuiViewport* Viewport, ImVec2 Position)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        Window->SetWindowPos(static_cast<int32>(Position.x), static_cast<int32>(Position.y));
    }
}

static ImVec2 ImGuiGetWindowSize(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        FWindowShape WindowShape;
        Window->GetWindowShape(WindowShape);

        const ImVec2 Result = ImVec2(static_cast<float>(WindowShape.Width), static_cast<float>(WindowShape.Height));
        return Result;
    }

    return ImVec2(0.0f, 0.0f);
}

static void ImGuiSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        const FWindowShape WindowShape(static_cast<uint32>(Size.x), static_cast<uint32>(Size.y));
        Window->SetWindowShape(WindowShape, false);
    }
}

static void ImGuiSetWindowFocus(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        Window->SetWindowFocus();
    }
}

static bool ImGuiGetWindowFocus(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->IsActiveWindow();
    }

    return false;
}

static bool ImGuiGetWindowMinimized(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->IsMinimized();
    }

    return false;
}

static void ImGuiSetWindowTitle(ImGuiViewport* Viewport, const char* Title)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->SetTitle(Title);
    }
}

static void ImGuiSetWindowAlpha(ImGuiViewport* Viewport, float Alpha)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->SetWindowOpacity(Alpha);
    }
}

static float ImGuiGetWindowDpiScale(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        return Window->GetWindowDpiScale();
    }

    return 1.0f;
}

static void ImGuiOnChangedViewport(ImGuiViewport*)
{
}


struct FEventDispatcher
{
public:
    class FLeafLastPolicy
    {
    public:
        FLeafLastPolicy(FFilteredWidgets& InWidgets)
            : Widgets(InWidgets)
            , Index(static_cast<uint32>(InWidgets.LastIndex()))
        {
        }

        bool ShouldProcess() const
        {
            return (Index > 0) && (Widgets.NumChildren() > 0);
        }

        void Next()
        {
            Index--;
        }
        
        TSharedPtr<FWidget>& GetWidget()
        {
            return Widgets[Index];
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[Index];
        }

    private:
        FFilteredWidgets& Widgets;
        int32             Index;
    };

    class FLeafFirstPolicy
    {
    public:
        FLeafFirstPolicy(FFilteredWidgets& InWidgets)
            : Widgets(InWidgets)
            , Index(0)
        {
        }

        bool ShouldProcess() const
        {
            return (Index < static_cast<int32>(Widgets.NumChildren()));
        }

        void Next()
        {
            Index++;
        }

        TSharedPtr<FWidget>& GetWidget()
        {
            return Widgets[Index];
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[Index];
        }

    private:
        FFilteredWidgets& Widgets;
        int32             Index;
    };

    class FDirectPolicy
    {
    public:
        FDirectPolicy(FFilteredWidgets& InWidgets)
            : Widgets(InWidgets)
            , bIsProcessed(false)
        {
        }

        bool ShouldProcess() const
        {
            return !bIsProcessed;
        }

        void Next()
        {
            bIsProcessed = true;
        }

        TSharedPtr<FWidget>& GetWidget()
        {
            return Widgets[0];
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[0];
        }

    private:
        FFilteredWidgets& Widgets;
        bool              bIsProcessed;
    };

public:
    template<typename PolicyType, typename EventType, typename PedicateType>
    static FResponse Dispatch(FWindowedApplication* Application, PolicyType Policy, const EventType& Event, PedicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            Response = Predicate(Application, Policy.GetWidget(), Event);
        }

        return Response;
    }
};


TSharedPtr<FWindowedApplication> FWindowedApplication::CurrentApplication  = nullptr;
TSharedPtr<FGenericApplication>  FWindowedApplication::PlatformApplication = nullptr;

bool FWindowedApplication::Create()
{
    PlatformApplication = TSharedPtr<FGenericApplication>(FPlatformApplicationMisc::CreateApplication());
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    CurrentApplication = MakeShared<FWindowedApplication>();
    PlatformApplication->SetMessageHandler(CurrentApplication);
    return true;
}

void FWindowedApplication::Destroy()
{
    if (CurrentApplication)
    {
        CurrentApplication->OverridePlatformApplication(nullptr);
        CurrentApplication.Reset();
    }

    if (PlatformApplication)
    {
        PlatformApplication->SetMessageHandler(nullptr);
        PlatformApplication.Reset();
    }
}

FWindowedApplication::FWindowedApplication()
    : MainViewport(nullptr)
    , Renderer(nullptr)
    , Windows()
    , InputHandlers()
    , Context(nullptr)
    , bIsTrackingMouse(false)
{
    IMGUI_CHECKVERSION();
    Context = ImGui::CreateContext();
    CHECK(Context != nullptr);

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendPlatformName = "DXR-Engine";

    // Configure ImGui
    UIState.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    UIState.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Application Flags
    UIState.BackendFlags |= ImGuiBackendFlags_HasGamepad;              // Platform supports gamepad and currently has one connected.
    UIState.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values
    UIState.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests
    UIState.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side
    UIState.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data
    
    // Renderer Flags
    UIState.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    UIState.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can call io.AddMouseViewportEvent() with correct data

    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        Viewport->PlatformWindowCreated = false;
    }

    // Init monitor information
    UpdateMonitorInfo();

    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    PlatformState.Platform_CreateWindow       = ImGuiCreateWindow;
    PlatformState.Platform_DestroyWindow      = ImGuiDestroyWindow;
    PlatformState.Platform_ShowWindow         = ImGuiShowWindow;
    PlatformState.Platform_SetWindowPos       = ImGuiSetWindowPos;
    PlatformState.Platform_GetWindowPos       = ImGuiGetWindowPos;
    PlatformState.Platform_SetWindowSize      = ImGuiSetWindowSize;
    PlatformState.Platform_GetWindowSize      = ImGuiGetWindowSize;
    PlatformState.Platform_SetWindowFocus     = ImGuiSetWindowFocus;
    PlatformState.Platform_GetWindowFocus     = ImGuiGetWindowFocus;
    PlatformState.Platform_GetWindowMinimized = ImGuiGetWindowMinimized;
    PlatformState.Platform_SetWindowTitle     = ImGuiSetWindowTitle;
    PlatformState.Platform_SetWindowAlpha     = ImGuiSetWindowAlpha;
    PlatformState.Platform_UpdateWindow       = ImGuiUpdateWindow;
    PlatformState.Platform_GetWindowDpiScale  = ImGuiGetWindowDpiScale;
    PlatformState.Platform_OnChangedViewport  = ImGuiOnChangedViewport;

    // Setup the style
    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    // Padding
    Style.FramePadding = ImVec2(6.0f, 4.0f);

    // Use AA for lines etc.
    Style.AntiAliasedLines = true;
    Style.AntiAliasedFill  = true;

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
}

FWindowedApplication::~FWindowedApplication()
{
    if (Context)
    {
        ImGui::DestroyContext(Context);
        Context = nullptr;
    }
}

bool FWindowedApplication::InitializeRenderer()
{
    Renderer = MakeUnique<FViewportRenderer>();
    if (!Renderer->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ViewportRenderer ");
        return false;
    }

    return true; 
}

void FWindowedApplication::ReleaseRenderer()
{
    Renderer.Reset();
}

void FWindowedApplication::Tick(FTimespan DeltaTime)
{
    // Update platform
    const float Delta = static_cast<float>(DeltaTime.AsMilliseconds());
    PlatformApplication->Tick(Delta);

    // TODO: Investigate if this is a problem
    if (!MainWindow)
    {
        return;
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.DeltaTime   = static_cast<float>(DeltaTime.AsSeconds());
    
    // Setup the display size of the Main-Window
    UIState.DisplaySize = ImVec2(static_cast<float>(MainWindow->GetWidth()), static_cast<float>(MainWindow->GetHeight()));
    // Setup the display scale from the Main-Window
    const float WindowDpiScale = MainWindow->GetWindowDpiScale();
    UIState.DisplayFramebufferScale = ImVec2(WindowDpiScale, WindowDpiScale);

    // Retrieve the current active window
    TSharedRef<FGenericWindow> ForegroundWindow = PlatformApplication->GetForegroundWindow();
    
    // Update Mouse
    ImGuiViewport* ForegroundViewport = nullptr;
    if (ForegroundWindow)
    {
        ForegroundViewport = ImGui::FindViewportByPlatformHandle(ForegroundWindow->GetPlatformHandle());
    }

    const bool bIsAppFocused = ForegroundWindow && (ForegroundWindow == MainWindow || MainWindow->IsChildWindow(ForegroundWindow) || ForegroundViewport);
    if (bIsAppFocused)
    {
        if (UIState.WantSetMousePos)
        {
            SetCursorPos(FIntVector2(static_cast<int32>(UIState.MousePos.x), static_cast<int32>(UIState.MousePos.y)));
        }
        else if (!UIState.WantSetMousePos && !bIsTrackingMouse)
        {
            const FIntVector2 CursorPos = GetCursorPos();
            UIState.AddMousePosEvent(CursorPos.x, CursorPos.y);
        }
    }

    ImGuiID MouseViewportID = 0;
    if (TSharedRef<FGenericWindow> WindowUnderCursor = PlatformApplication->GetWindowUnderCursor())
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(WindowUnderCursor->GetPlatformHandle()))
        {
            MouseViewportID = Viewport->ID;
        }
    }

    UIState.AddMouseViewportEvent(MouseViewportID);

    // Update the cursor type
    const bool bNoMouseCursorChange = (UIState.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0;
    if (!bNoMouseCursorChange)
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if (ImguiCursor == ImGuiMouseCursor_None || UIState.MouseDrawCursor)
        {
            SetCursor(ECursor::None);
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

            SetCursor(Cursor);
        }
    }

    // Poll input devices
    PollInputDevices();

    UIState.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (IsGamePadConnected())
    {
        UIState.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }

    // Update all the UI windows
    ImGui::NewFrame();

    //Windows.Foreach([](TSharedRef<FWidget>& Window)
    //{
    //    Window->Tick();
    //});

    static bool ShowDemoWindow    = true;
    static bool ShowAnotherWindow = false;
    static ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    if (ShowDemoWindow)
    {
        ImGui::ShowDemoWindow();
    }

    {
        static float SliderValue = 0.0f;
        static int32 Counter     = 0;

        ImGui::Begin("Hello, world!");

        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &ShowDemoWindow);
        ImGui::Checkbox("Another Window", &ShowAnotherWindow);

        ImGui::SliderFloat("float", &SliderValue, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&ClearColor);

        if (ImGui::Button("Button"))
        {
            Counter++;
        }

        ImGui::SameLine();
        ImGui::Text("counter = %d", Counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    if (ShowAnotherWindow)
    {
        ImGui::Begin("Another Window", &ShowAnotherWindow);
        ImGui::Text("Hello from another window!");

        if (ImGui::Button("Close Me"))
        {
            ShowAnotherWindow = false;
        }

        ImGui::End();
    }

    ImGui::EndFrame();
}

void FWindowedApplication::PollInputDevices()
{
    PlatformApplication->PollInputDevices();
}

void FWindowedApplication::UpdateMonitorInfo()
{
    PlatformApplication->GetDisplayInfo(DisplayInfo);

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

bool FWindowedApplication::OnControllerButtonUp(EControllerButton Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, false, ControllerIndex);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerButtonUpEvent(ControllerEvent))
        {
            return true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    const ImGuiKey GamepadButton = ImGui_GetGamepadButton(ControllerEvent.GetButton());
    if (GamepadButton != ImGuiKey_None)
    {
        UIState.AddKeyEvent(GamepadButton, ControllerEvent.IsButtonDown());
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonUp(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnControllerButtonDown(EControllerButton Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, true, ControllerIndex);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerButtonDownEvent(ControllerEvent))
        {
            return true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    const ImGuiKey GamepadButton = ImGui_GetGamepadButton(ControllerEvent.GetButton());
    if (GamepadButton != ImGuiKey_None)
    {
        UIState.AddKeyEvent(GamepadButton, ControllerEvent.IsButtonDown());
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonDown(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnControllerAnalog(EControllerAnalog AnalogSource, uint32 ControllerIndex, float AnalogValue)
{
    // Create the event
    const FControllerEvent ControllerEvent(AnalogSource, AnalogValue, ControllerIndex);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerAnalogEvent(ControllerEvent))
        {
            return true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();

    const bool bIsNegative = ControllerEvent.GetAnalogValue() < 0.0f;
    const ImGuiKey GamepadButton = ImGui_GetGamepadAnalog(ControllerEvent.GetAnalogSource(), bIsNegative);
    if (GamepadButton != ImGuiKey_None)
    {
        const float Normalized = FMath::Abs<float>(ControllerEvent.GetAnalogValue());
        UIState.AddKeyAnalogEvent(GamepadButton, Normalized > 0.10f, Normalized);
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonAnalog(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState)
{
    FResponse Response = FResponse::Unhandled();

    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, false, false);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyDownEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Remove the Key
    PressedKeys.erase(KeyCode);

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const FModifierKeyState ModifierKeyState = FPlatformApplicationMisc::GetModifierKeyState();
    UIState.AddKeyEvent(ImGuiMod_Ctrl , ModifierKeyState.bIsCtrlDown  == 1);
    UIState.AddKeyEvent(ImGuiMod_Shift, ModifierKeyState.bIsShiftDown == 1);
    UIState.AddKeyEvent(ImGuiMod_Alt  , ModifierKeyState.bIsAltDown   == 1);
    UIState.AddKeyEvent(ImGuiMod_Super, ModifierKeyState.bIsSuperDown == 1);

    const ImGuiKey Key = ImGui_KeyToImGuiKey(KeyEvent.GetKey());
    if (Key != ImGuiKey_None)
    {
        UIState.AddKeyEvent(Key, KeyEvent.IsDown());
    }

    if (UIState.WantCaptureKeyboard)
    {
        Response = FResponse::Handled();
    }

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    FResponse Response = FResponse::Unhandled();

    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, bIsRepeat, true);
    
    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyDownEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Add the Key among the pressed keys
    PressedKeys.insert(KeyCode);

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const FModifierKeyState ModifierKeyState = FPlatformApplicationMisc::GetModifierKeyState();
    UIState.AddKeyEvent(ImGuiMod_Ctrl , ModifierKeyState.bIsCtrlDown  == 1);
    UIState.AddKeyEvent(ImGuiMod_Shift, ModifierKeyState.bIsShiftDown == 1);
    UIState.AddKeyEvent(ImGuiMod_Alt  , ModifierKeyState.bIsAltDown   == 1);
    UIState.AddKeyEvent(ImGuiMod_Super, ModifierKeyState.bIsSuperDown == 1);

    const ImGuiKey Key = ImGui_KeyToImGuiKey(KeyEvent.GetKey());
    if (Key != ImGuiKey_None)
    {
        UIState.AddKeyEvent(Key, KeyEvent.IsDown());
    }

    if (UIState.WantCaptureKeyboard)
    {
        Response = FResponse::Handled();
    }

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnKeyChar(uint32 Character)
{
    FResponse Response = FResponse::Unhandled();

    // Create the event
    const FKeyEvent KeyEvent(FPlatformApplicationMisc::GetModifierKeyState(), Key_Unknown, Character, false, true);
    
    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyCharEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(KeyEvent.GetAnsiChar());

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyChar(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseMove(int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, FPlatformApplicationMisc::GetModifierKeyState());
    
    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseMove(MouseEvent))
        {
            return true;
        }
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    // Assume that we are dragging since a MouseButton is pressed
    const bool bIsNotDragging = PressedMouseButtons.empty();

    // If the Widgets is not in the newly pressed widgets, but is tracked then remove it
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& Widget = TrackedWidgets[Index];
        if (!Children.Contains(Widget) && bIsNotDragging)
        {
            Widget->OnMouseLeft(MouseEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            ++Index;
        }
    }

    // Update ImGui mouse
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));

    // If the tracked widgets contain the widget, send the events that the mouse entered the widgets
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            if (!Application->TrackedWidgets.Contains(Widget))
            {
                Application->TrackedWidgets.Add(Widget);
                Widget->OnMouseEntered(MouseEvent);
            }

            return FResponse::Unhandled();
        });

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& KeyEvent)
        {
            return Widget->OnMouseMove(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseButtonUp(EMouseButton Button, FModifierKeyState ModiferKeyState, int32 x, int32 y)
{
    // Remove the mouse capture if there is a capture
    SetCapture(nullptr);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, ModiferKeyState, Button, false);
    
    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseButtonUpEvent(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = ImGui_GetMouseButtonIndex(MouseEvent.GetButton());
    UIState.AddMouseButtonEvent(ButtonIndex, MouseEvent.IsDown());

    if (UIState.WantCaptureMouse)
    {
        Response = FResponse::Handled();
    }

    // Remove the Key
    PressedMouseButtons.erase(Button);

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    // If the Widgets is not in the newly pressed widgets, but is tracked then remove it
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& Widget = TrackedWidgets[Index];
        if (!Children.Contains(Widget))
        {
            Widget->OnMouseLeft(MouseEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            ++Index;
        }
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            return Widget->OnMouseButtonUp(MouseEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
{
    // Set the mouse capture when the mouse is pressed
    SetCapture(Window);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, ModierKeyState, Button, true);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseButtonDownEvent(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = ImGui_GetMouseButtonIndex(MouseEvent.GetButton());
    UIState.AddMouseButtonEvent(ButtonIndex, MouseEvent.IsDown());

    if (UIState.WantCaptureMouse)
    {
        Response = FResponse::Handled();
    }

    // Remove the Key
    PressedMouseButtons.insert(Button);

    // Store the window we focus on
    FocusWindow = PlatformApplication->GetActiveWindow();

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            const FResponse Response = Widget->OnMouseButtonDown(MouseEvent);
            if (Response.IsEventHandled())
            {
                if (!Application->TrackedWidgets.Contains(Widget))
                {
                    Application->TrackedWidgets.Add(Widget);
                }
            }

            return Response;
        });

    FocusPath = Children;
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseScrolled(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    if (MouseEvent.IsVerticalScrollDelta())
    {
        UIState.AddMouseWheelEvent(0.0f, MouseEvent.GetScrollDelta());
    }
    else
    {
        UIState.AddMouseWheelEvent(MouseEvent.GetScrollDelta(), 0.0f);
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            // TODO: return Widget->OnMouseScrolled(MouseEvent);
            return FResponse::Unhandled();
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnWindowResized(const TSharedRef<FGenericWindow>& InWindow, uint32 Width, uint32 Height)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(InWindow->GetPlatformHandle()))
    {
        Viewport->PlatformRequestResize = true;
    }

    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        const FWindowResizedEvent WindowResizeEvent(Width, Height);
        const FResponse Response = Window->OnWindowResized(WindowResizeEvent);
        if (Response.IsEventHandled())
        {
            return true;
        }
    }

    return false;
}

bool FWindowedApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& InWindow, int32 x, int32 y)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(InWindow->GetPlatformHandle()))
    {
        Viewport->PlatformRequestMove = true;
    }

    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        const FWindowMovedEvent WindowsMovedEvent(FIntVector2{ x, y });
        const FResponse Response = Window->OnWindowMoved(WindowsMovedEvent);
        if (Response.IsEventHandled())
        {
            return true;
        }
    }

    return false;
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

bool FWindowedApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& InWindow)
{
    // The state needs to be reset when the window loses focus
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddFocusEvent(false);
    return true;
}

bool FWindowedApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& InWindow)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddFocusEvent(true);
    return true;
}

bool FWindowedApplication::OnWindowMouseLeft(const TSharedRef<FGenericWindow>& InWindow)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddMousePosEvent(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
    return true;
}

bool FWindowedApplication::OnWindowMouseEntered(const TSharedRef<FGenericWindow>& InWindow)
{
    return false;
}

bool FWindowedApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& InWindow)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(InWindow->GetPlatformHandle()))
    {
        Viewport->PlatformRequestClose = true;
    }

    if (FGenericWindow* Window = MainViewport->GetWindow())
    {
        if (Window == InWindow)
        {
            FPlatformApplicationMisc::RequestExit(0);
            MainViewport = nullptr;
        }
    }

    // Remove the viewport
    for (int32 Index = 0; Index < Viewports.Size(); Index++)
    {
        if (Viewports[Index]->GetWindow() == InWindow)
        {
            Viewports.RemoveAt(Index);
            break;
        }
    }

    // Remove the window
    AllWindows.Remove(InWindow);
    return true;
}

bool FWindowedApplication::OnMonitorChange()
{
    UpdateMonitorInfo();
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

TSharedRef<FGenericWindow> FWindowedApplication::CreateWindow(const FWindowInitializer& Initializer)
{
    TSharedRef<FGenericWindow> Window = PlatformApplication->CreateWindow();
    if (Window)
    {
        if (Window->Initialize(Initializer.Title, Initializer.Width, Initializer.Height, Initializer.Position.x, Initializer.Position.y, Initializer.Style, Initializer.ParentWindow))
        {
            AllWindows.Add(Window);
            return Window;
        }
    }

    return nullptr;
}

TSharedPtr<FViewport> FWindowedApplication::CreateViewport(const FViewportInitializer& Initializer)
{
    TSharedPtr<FViewport> Viewport = MakeShared<FViewport>();
    if (Viewport)
    {
        if (Viewport->InitializeRHI(Initializer))
        {
            Viewports.Add(Viewport);
            return Viewport;
        }
    }

    return nullptr;
}

void FWindowedApplication::SetCursor(ECursor InCursor)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetCursor(InCursor);
    }
}

void FWindowedApplication::SetCursorPos(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

FIntVector2 FWindowedApplication::GetCursorPos() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->GetPosition();
    }

    return FIntVector2();
}

void FWindowedApplication::ShowCursor(bool bIsVisible)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetVisibility(bIsVisible);
    }
}

bool FWindowedApplication::IsCursorVisibile() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->IsVisible();
    }

    return false;
}

bool FWindowedApplication::IsGamePadConnected() const
{
    if (FInputDevice* InputDevice = GetInputDeviceInterface())
    {
        return InputDevice->IsDeviceConnected();
    }

    return false;
}

bool FWindowedApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window) 
{ 
    if (Window)
    {
        TSharedRef<FGenericWindow> NativeWindow = Window->GetNativeWindow();
        return PlatformApplication->EnableHighPrecisionMouseForWindow(NativeWindow);
    }

    return false;
}

void FWindowedApplication::SetCapture(const TSharedRef<FGenericWindow>& CaptureWindow)
{
    PlatformApplication->SetCapture(CaptureWindow);
    if (CaptureWindow && !PressedMouseButtons.empty())
    {
        bIsTrackingMouse = true;
    }
    else
    {
        bIsTrackingMouse = false;
    }
}

void FWindowedApplication::SetActiveWindow(const TSharedRef<FGenericWindow>& ActiveWindow)
{
    if (ActiveWindow)
    {
        PlatformApplication->SetActiveWindow(ActiveWindow);
    }
}

TSharedPtr<FWindow> FWindowedApplication::GetActiveWindow() const 
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetActiveWindow();
    return FindWindowFromNativeWindow(NativeWindow);
}

TSharedPtr<FWindow> FWindowedApplication::GetWindowUnderCursor() const
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetActiveWindow();
    return FindWindowFromNativeWindow(NativeWindow);
}

TSharedRef<FGenericWindow> FWindowedApplication::GetCapture() const
{ 
    return PlatformApplication->GetCapture();
}

void FWindowedApplication::AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 NewPriority)
{
    FPriorityInputHandler NewPair(NewInputHandler, NewPriority);
    if (!InputHandlers.Contains(NewPair))
    {
        for (int32 Index = 0; Index < InputHandlers.Size(); )
        {
            const FPriorityInputHandler& Handler = InputHandlers[Index];
            if (NewPriority <= Handler.Priority)
            {
                Index++;
                InputHandlers.Insert(Index, NewPair);
                return;
            }
        }

        InputHandlers.Add(NewPair);
    }
}

void FWindowedApplication::RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler Handler = InputHandlers[Index];
        if (Handler.InputHandler == InputHandler)
        {
            InputHandlers.RemoveAt(Index);
            return;
        }
    }
}

void FWindowedApplication::RegisterMainViewport(const TSharedPtr<FViewport>& InViewport)
{
    if (MainViewport != InViewport)
    {
        MainViewport = InViewport;
        MainWindow   = MakeSharedRef<FGenericWindow>(MainViewport->GetWindow());

        if (MainWindow)
        {
            if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
            {
                // Viewport is now created, make sure we get the initial information of the window
                Viewport->PlatformWindowCreated = true;
                Viewport->PlatformRequestMove   = true;
                Viewport->PlatformRequestResize = true;

                // Set native handles
                Viewport->PlatformUserData = MainWindow.Get();
                Viewport->RendererUserData = InViewport.Get();
                Viewport->PlatformHandle   = Viewport->PlatformHandleRaw = MainWindow->GetPlatformHandle();
            }
        }
    }
}

void FWindowedApplication::AddWindow(const TSharedPtr<FWindow>& NewWindow)
{
    if (NewWindow && !Windows.Contains(NewWindow))
    {
        Windows.Emplace(NewWindow);
    }
}

void FWindowedApplication::RemoveWindow(const TSharedPtr<FWindow>& Window)
{
    if (Window)
    {
        Windows.Remove(Window);
    }
}

void FWindowedApplication::DrawWindows(FRHICommandList& CommandList)
{
    // NOTE: Renderer is not forced to be valid
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

TSharedPtr<FWindow> FWindowedApplication::FindWindowFromNativeWindow(const TSharedRef<FGenericWindow>& NativeWindow) const
{
    if (NativeWindow)
    {
        for (const TSharedPtr<FWindow>& Window : Windows)
        {
            if (NativeWindow == Window->GetNativeWindow())
            {
                return Window;
            }
        }
    }

    return nullptr;
}

TSharedPtr<FWindow> FWindowedApplication::FindWindowUnderCursor()
{
    return FindWindowFromNativeWindow(PlatformApplication->GetWindowUnderCursor());
}

void FWindowedApplication::FindWidgetsUnderCursor(FIntVector2 CursorPos, FFilteredWidgets& OutWidgets)
{
    if (TSharedPtr<FWindow> Window = FindWindowUnderCursor())
    {
        Window->FindWidgetsUnderCursor(CursorPos, OutWidgets);
    }
}

void FWindowedApplication::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
{
    // Set a MessageHandler to avoid any potential nullptr access
    PlatformApplication->SetMessageHandler(MakeShared<FGenericApplicationMessageHandler>());

    if (InPlatformApplication)
    {
        CHECK(PlatformApplication != InPlatformApplication);
        InPlatformApplication->SetMessageHandler(CurrentApplication);
    }

    PlatformApplication = InPlatformApplication;
}
