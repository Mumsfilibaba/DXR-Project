#include "ImGuiModule.h"
#include "Application.h"
#include "Input/InputMapper.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

// TODO: Turn this into lookup tables
static uint32 ImGui_GetMouseButtonIndex(EMouseButtonName::Type Button)
{
    switch (Button)
    {
    case EMouseButtonName::Left:   return ImGuiMouseButton_Left;
    case EMouseButtonName::Right:  return ImGuiMouseButton_Right;
    case EMouseButtonName::Middle: return ImGuiMouseButton_Middle;
    case EMouseButtonName::Thumb1: return 3;
    case EMouseButtonName::Thumb2: return 4;
    default:                       return ImGuiKey_None;
    }
}

static ImGuiKey ImGui_KeyToImGuiKey(EKeyboardKeyName::Type Key)
{
    switch (Key)
    {
    case EKeyboardKeyName::Tab:            return ImGuiKey_Tab;
    case EKeyboardKeyName::Left:           return ImGuiKey_LeftArrow;
    case EKeyboardKeyName::Right:          return ImGuiKey_RightArrow;
    case EKeyboardKeyName::Up:             return ImGuiKey_UpArrow;
    case EKeyboardKeyName::Down:           return ImGuiKey_DownArrow;
    case EKeyboardKeyName::PageUp:         return ImGuiKey_PageUp;
    case EKeyboardKeyName::PageDown:       return ImGuiKey_PageDown;
    case EKeyboardKeyName::Home:           return ImGuiKey_Home;
    case EKeyboardKeyName::End:            return ImGuiKey_End;
    case EKeyboardKeyName::Insert:         return ImGuiKey_Insert;
    case EKeyboardKeyName::Delete:         return ImGuiKey_Delete;
    case EKeyboardKeyName::Backspace:      return ImGuiKey_Backspace;
    case EKeyboardKeyName::Space:          return ImGuiKey_Space;
    case EKeyboardKeyName::Enter:          return ImGuiKey_Enter;
    case EKeyboardKeyName::Escape:         return ImGuiKey_Escape;
    case EKeyboardKeyName::Apostrophe:     return ImGuiKey_Apostrophe;
    case EKeyboardKeyName::Comma:          return ImGuiKey_Comma;
    case EKeyboardKeyName::Minus:          return ImGuiKey_Minus;
    case EKeyboardKeyName::Period:         return ImGuiKey_Period;
    case EKeyboardKeyName::Slash:          return ImGuiKey_Slash;
    case EKeyboardKeyName::Semicolon:      return ImGuiKey_Semicolon;
    case EKeyboardKeyName::Equal:          return ImGuiKey_Equal;
    case EKeyboardKeyName::LeftBracket:    return ImGuiKey_LeftBracket;
    case EKeyboardKeyName::Backslash:      return ImGuiKey_Backslash;
    case EKeyboardKeyName::RightBracket:   return ImGuiKey_RightBracket;
    case EKeyboardKeyName::GraveAccent:    return ImGuiKey_GraveAccent;
    case EKeyboardKeyName::CapsLock:       return ImGuiKey_CapsLock;
    case EKeyboardKeyName::ScrollLock:     return ImGuiKey_ScrollLock;
    case EKeyboardKeyName::NumLock:        return ImGuiKey_NumLock;
    case EKeyboardKeyName::PrintScreen:    return ImGuiKey_PrintScreen;
    case EKeyboardKeyName::Pause:          return ImGuiKey_Pause;
    case EKeyboardKeyName::KeypadZero:     return ImGuiKey_Keypad0;
    case EKeyboardKeyName::KeypadOne:      return ImGuiKey_Keypad1;
    case EKeyboardKeyName::KeypadTwo:      return ImGuiKey_Keypad2;
    case EKeyboardKeyName::KeypadThree:    return ImGuiKey_Keypad3;
    case EKeyboardKeyName::KeypadFour:     return ImGuiKey_Keypad4;
    case EKeyboardKeyName::KeypadFive:     return ImGuiKey_Keypad5;
    case EKeyboardKeyName::KeypadSix:      return ImGuiKey_Keypad6;
    case EKeyboardKeyName::KeypadSeven:    return ImGuiKey_Keypad7;
    case EKeyboardKeyName::KeypadEight:    return ImGuiKey_Keypad8;
    case EKeyboardKeyName::KeypadNine:     return ImGuiKey_Keypad9;
    case EKeyboardKeyName::KeypadDecimal:  return ImGuiKey_KeypadDecimal;
    case EKeyboardKeyName::KeypadDivide:   return ImGuiKey_KeypadDivide;
    case EKeyboardKeyName::KeypadMultiply: return ImGuiKey_KeypadMultiply;
    case EKeyboardKeyName::KeypadSubtract: return ImGuiKey_KeypadSubtract;
    case EKeyboardKeyName::KeypadAdd:      return ImGuiKey_KeypadAdd;
    case EKeyboardKeyName::KeypadEnter:    return ImGuiKey_KeypadEnter;
    case EKeyboardKeyName::LeftShift:      return ImGuiKey_LeftShift;
    case EKeyboardKeyName::LeftControl:    return ImGuiKey_LeftCtrl;
    case EKeyboardKeyName::LeftAlt:        return ImGuiKey_LeftAlt;
    case EKeyboardKeyName::LeftSuper:      return ImGuiKey_LeftSuper;
    case EKeyboardKeyName::RightShift:     return ImGuiKey_RightShift;
    case EKeyboardKeyName::RightControl:   return ImGuiKey_RightCtrl;
    case EKeyboardKeyName::RightAlt:       return ImGuiKey_RightAlt;
    case EKeyboardKeyName::RightSuper:     return ImGuiKey_RightSuper;
    case EKeyboardKeyName::Menu:           return ImGuiKey_Menu;
    case EKeyboardKeyName::Zero:           return ImGuiKey_0;
    case EKeyboardKeyName::One:            return ImGuiKey_1;
    case EKeyboardKeyName::Two:            return ImGuiKey_2;
    case EKeyboardKeyName::Three:          return ImGuiKey_3;
    case EKeyboardKeyName::Four:           return ImGuiKey_4;
    case EKeyboardKeyName::Five:           return ImGuiKey_5;
    case EKeyboardKeyName::Six:            return ImGuiKey_6;
    case EKeyboardKeyName::Seven:          return ImGuiKey_7;
    case EKeyboardKeyName::Eight:          return ImGuiKey_8;
    case EKeyboardKeyName::Nine:           return ImGuiKey_9;
    case EKeyboardKeyName::A:              return ImGuiKey_A;
    case EKeyboardKeyName::B:              return ImGuiKey_B;
    case EKeyboardKeyName::C:              return ImGuiKey_C;
    case EKeyboardKeyName::D:              return ImGuiKey_D;
    case EKeyboardKeyName::E:              return ImGuiKey_E;
    case EKeyboardKeyName::F:              return ImGuiKey_F;
    case EKeyboardKeyName::G:              return ImGuiKey_G;
    case EKeyboardKeyName::H:              return ImGuiKey_H;
    case EKeyboardKeyName::I:              return ImGuiKey_I;
    case EKeyboardKeyName::J:              return ImGuiKey_J;
    case EKeyboardKeyName::K:              return ImGuiKey_K;
    case EKeyboardKeyName::L:              return ImGuiKey_L;
    case EKeyboardKeyName::M:              return ImGuiKey_M;
    case EKeyboardKeyName::N:              return ImGuiKey_N;
    case EKeyboardKeyName::O:              return ImGuiKey_O;
    case EKeyboardKeyName::P:              return ImGuiKey_P;
    case EKeyboardKeyName::Q:              return ImGuiKey_Q;
    case EKeyboardKeyName::R:              return ImGuiKey_R;
    case EKeyboardKeyName::S:              return ImGuiKey_S;
    case EKeyboardKeyName::T:              return ImGuiKey_T;
    case EKeyboardKeyName::U:              return ImGuiKey_U;
    case EKeyboardKeyName::V:              return ImGuiKey_V;
    case EKeyboardKeyName::W:              return ImGuiKey_W;
    case EKeyboardKeyName::X:              return ImGuiKey_X;
    case EKeyboardKeyName::Y:              return ImGuiKey_Y;
    case EKeyboardKeyName::Z:              return ImGuiKey_Z;
    case EKeyboardKeyName::F1:             return ImGuiKey_F1;
    case EKeyboardKeyName::F2:             return ImGuiKey_F2;
    case EKeyboardKeyName::F3:             return ImGuiKey_F3;
    case EKeyboardKeyName::F4:             return ImGuiKey_F4;
    case EKeyboardKeyName::F5:             return ImGuiKey_F5;
    case EKeyboardKeyName::F6:             return ImGuiKey_F6;
    case EKeyboardKeyName::F7:             return ImGuiKey_F7;
    case EKeyboardKeyName::F8:             return ImGuiKey_F8;
    case EKeyboardKeyName::F9:             return ImGuiKey_F9;
    case EKeyboardKeyName::F10:            return ImGuiKey_F10;
    case EKeyboardKeyName::F11:            return ImGuiKey_F11;
    case EKeyboardKeyName::F12:            return ImGuiKey_F12;
    default:                       return ImGuiKey_None;
    }
}

static ImGuiKey ImGui_GetGamepadButton(EGamepadButtonName::Type Button)
{
    switch (Button)
    {
    case EGamepadButtonName::Start:         return ImGuiKey_GamepadStart;
    case EGamepadButtonName::Back:          return ImGuiKey_GamepadBack;
    case EGamepadButtonName::DPadUp:        return ImGuiKey_GamepadDpadUp;
    case EGamepadButtonName::DPadDown:      return ImGuiKey_GamepadDpadDown;
    case EGamepadButtonName::DPadLeft:      return ImGuiKey_GamepadDpadLeft;
    case EGamepadButtonName::DPadRight:     return ImGuiKey_GamepadDpadRight;
    case EGamepadButtonName::FaceUp:        return ImGuiKey_GamepadFaceUp;
    case EGamepadButtonName::FaceDown:      return ImGuiKey_GamepadFaceDown;
    case EGamepadButtonName::FaceLeft:      return ImGuiKey_GamepadFaceLeft;
    case EGamepadButtonName::FaceRight:     return ImGuiKey_GamepadFaceRight;
    case EGamepadButtonName::RightTrigger:  return ImGuiKey_GamepadR3;
    case EGamepadButtonName::LeftTrigger:   return ImGuiKey_GamepadL3;
    case EGamepadButtonName::RightShoulder: return ImGuiKey_GamepadR3;
    case EGamepadButtonName::LeftShoulder:  return ImGuiKey_GamepadL3;
    default:                               return ImGuiKey_None;
    }
}

static ImGuiKey ImGui_GetGamepadAnalog(EAnalogSourceName::Type Analog, bool bIsNegative)
{
    switch (Analog)
    {
    case EAnalogSourceName::RightThumbX:  return bIsNegative ? ImGuiKey_GamepadRStickDown : ImGuiKey_GamepadRStickUp;
    case EAnalogSourceName::RightThumbY:  return bIsNegative ? ImGuiKey_GamepadRStickLeft : ImGuiKey_GamepadRStickRight;
    case EAnalogSourceName::LeftThumbX:   return bIsNegative ? ImGuiKey_GamepadLStickDown : ImGuiKey_GamepadLStickUp;
    case EAnalogSourceName::LeftThumbY:   return bIsNegative ? ImGuiKey_GamepadLStickLeft : ImGuiKey_GamepadLStickRight;
    case EAnalogSourceName::RightTrigger: return ImGuiKey_GamepadR2;
    case EAnalogSourceName::LeftTrigger:  return ImGuiKey_GamepadL2;
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
            ParentWindow = reinterpret_cast<FGenericWindow*>(ParentViewport->PlatformUserData);
        }
    }

    TSharedRef<FGenericWindow> Window = FApplication::Get().CreateWindow(
        FGenericWindowInitializer()
        .SetTitle("Window")
        .SetWidth(static_cast<uint32>(Viewport->Size.x))
        .SetHeight(static_cast<uint32>(Viewport->Size.y))
        .SetPosition(FIntVector2(static_cast<int32>(Viewport->Pos.x), static_cast<int32>(Viewport->Pos.y)))
        .SetStyle(WindowStyle)
        .SetParentWindow(ParentWindow));

    if (Window)
    {
        Viewport->PlatformHandle = Viewport->PlatformHandleRaw = Window->GetPlatformHandle();
        Viewport->PlatformUserData = reinterpret_cast<void*>(Window.ReleaseOwnership());
        Viewport->PlatformRequestResize = false;
    }
}

static void ImGuiDestroyWindow(ImGuiViewport* Viewport)
{
    if (FGenericWindow* Window = reinterpret_cast<FGenericWindow*>(Viewport->PlatformUserData))
    {
        if (Window == FApplication::Get().GetCapture())
        {
            // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
            FApplication::Get().SetCapture(FApplication::Get().GetMainWindow());
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


ImGuiContext* FImGui::Context = nullptr;

void FImGui::CreateContext()
{
    IMGUI_CHECKVERSION();
    Context = ImGui::CreateContext();
    CHECK(Context != nullptr);

    // Application Flags
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendFlags |= ImGuiBackendFlags_HasGamepad;              // Platform supports Gamepad and currently has one connected.
    UIState.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values
    UIState.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests
    UIState.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create Multi-Viewports on the Platform side
    UIState.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data

    // Renderer Flags
    UIState.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    UIState.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can call io.AddMouseViewportEvent() with correct data

    // Ensure that the Main-Viewport is setup correctly
    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        Viewport->PlatformWindowCreated = false;
    }

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
}

void FImGui::DestroyContext()
{
    if (Context)
    {
        ImGui::DestroyContext(Context);
        Context = nullptr;
    }
}

void FImGui::InitializeStyle()
{
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

void FImGui::SetupMainViewport(FViewport* InViewport)
{
    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        if (InViewport)
        {
            // Viewport is now created, make sure we get the initial information of the window
            Viewport->PlatformWindowCreated = true;
            Viewport->PlatformRequestMove   = true;
            Viewport->PlatformRequestResize = true;

            // Set native handles
            TSharedRef<FGenericWindow> MainWindow = InViewport->GetWindow();
            Viewport->PlatformUserData = MainWindow.Get();
            Viewport->PlatformHandle = Viewport->PlatformHandleRaw = MainWindow->GetPlatformHandle();

            FViewportData* ViewportData = new FViewportData();
            ViewportData->Viewport = InViewport->GetRHIViewport();
            Viewport->RendererUserData = ViewportData;
        }
        else
        {
            Viewport->PlatformWindowCreated = false;
            Viewport->PlatformRequestMove   = false;
            Viewport->PlatformRequestResize = false;
            Viewport->PlatformUserData      = nullptr;
            Viewport->RendererUserData      = nullptr;
            Viewport->PlatformHandle        = Viewport->PlatformHandleRaw = nullptr;
        }
    }
}

FResponse FImGui::OnGamepadButtonEvent(FKey Key, bool bIsDown)
{
    const EGamepadButtonName::Type Button = FInputMapper::Get().GetGamepadButtonNameFromKey(Key);

    const ImGuiKey GamepadButton = ImGui_GetGamepadButton(Button);
    if (GamepadButton != ImGuiKey_None)
    {
        ImGui::GetIO().AddKeyEvent(GamepadButton, bIsDown);
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnGamepadAnalogEvent(EAnalogSourceName::Type AnalogSource, float Analog)
{
    const bool bIsNegative = Analog < 0.0f;

    const ImGuiKey GamepadButton = ImGui_GetGamepadAnalog(AnalogSource, bIsNegative);
    if (GamepadButton != ImGuiKey_None)
    {
        const float Normalized = FMath::Abs<float>(Analog);
        ImGui::GetIO().AddKeyAnalogEvent(GamepadButton, Normalized > 0.10f, Normalized);
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnKeyEvent(FKey InKey, FModifierKeyState ModifierKeyState, bool bIsDown)
{
    const EKeyboardKeyName::Type KeyName = FInputMapper::Get().GetKeyboardKeyNameFromKey(InKey);

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddKeyEvent(ImGuiMod_Ctrl , ModifierKeyState.bIsCtrlDown  == 1);
    UIState.AddKeyEvent(ImGuiMod_Shift, ModifierKeyState.bIsShiftDown == 1);
    UIState.AddKeyEvent(ImGuiMod_Alt  , ModifierKeyState.bIsAltDown   == 1);
    UIState.AddKeyEvent(ImGuiMod_Super, ModifierKeyState.bIsSuperDown == 1);

    const ImGuiKey Key = ImGui_KeyToImGuiKey(KeyName);
    if (Key != ImGuiKey_None)
    {
        UIState.AddKeyEvent(Key, bIsDown);
    }

    if (UIState.WantCaptureKeyboard)
    {
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnKeyCharEvent(uint32 Character)
{
    ImGui::GetIO().AddInputCharacter(Character);
    return FResponse::Unhandled();
}

FResponse FImGui::OnMouseMoveEvent(int32 x, int32 y)
{
    ImGui::GetIO().AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));
    return FResponse::Unhandled();
}

FResponse FImGui::OnMouseButtonEvent(FKey InKey, bool bIsDown)
{
    const EMouseButtonName::Type ButtonName = FInputMapper::Get().GetMouseButtonNameFromKey(InKey);

    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = ImGui_GetMouseButtonIndex(ButtonName);
    UIState.AddMouseButtonEvent(ButtonIndex, bIsDown);

    if (UIState.WantCaptureMouse)
    {
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnMouseScrollEvent(float ScrollDelta, bool bVertical)
{
    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    if (bVertical)
    {
        UIState.AddMouseWheelEvent(0.0f, ScrollDelta);
    }
    else
    {
        UIState.AddMouseWheelEvent(ScrollDelta, 0.0f);
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnWindowResize(void* PlatformHandle)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
    {
        Viewport->PlatformRequestResize = true;
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnWindowMoved(void* PlatformHandle) 
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
    {
        Viewport->PlatformRequestMove = true;
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnFocusLost()
{
    ImGui::GetIO().AddFocusEvent(false);
    return FResponse::Unhandled();
}

FResponse FImGui::OnFocusGained()
{
    ImGui::GetIO().AddFocusEvent(true);
    return FResponse::Unhandled();
}

FResponse FImGui::OnMouseLeft()
{
    ImGui::GetIO().AddMousePosEvent(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
    return FResponse::Unhandled();
}

FResponse FImGui::OnWindowClose(void* PlatformHandle)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(PlatformHandle))
    {
        Viewport->PlatformRequestClose = true;
    }

    return FResponse::Unhandled();
}