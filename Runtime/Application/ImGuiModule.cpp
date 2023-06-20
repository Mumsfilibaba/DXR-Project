#include "ImGuiModule.h"
#include "Application.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

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

FResponse FImGui::OnGamepadButtonEvent(EControllerButton Button, bool bIsDown)
{
    const ImGuiKey GamepadButton = ImGui_GetGamepadButton(Button);
    if (GamepadButton != ImGuiKey_None)
    {
        ImGui::GetIO().AddKeyEvent(GamepadButton, bIsDown);
    }

    return FResponse::Unhandled();
}

FResponse FImGui::OnGamepadAnalogEvent(EControllerAnalog AnalogSource, float Analog)
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

FResponse FImGui::OnKeyEvent(EKey InKey, bool bIsDown)
{
    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const FModifierKeyState ModifierKeyState = FPlatformApplicationMisc::GetModifierKeyState();
    UIState.AddKeyEvent(ImGuiMod_Ctrl , ModifierKeyState.bIsCtrlDown  == 1);
    UIState.AddKeyEvent(ImGuiMod_Shift, ModifierKeyState.bIsShiftDown == 1);
    UIState.AddKeyEvent(ImGuiMod_Alt  , ModifierKeyState.bIsAltDown   == 1);
    UIState.AddKeyEvent(ImGuiMod_Super, ModifierKeyState.bIsSuperDown == 1);

    const ImGuiKey Key = ImGui_KeyToImGuiKey(InKey);
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

FResponse FImGui::OnMouseButtonEvent(EMouseButton Button, bool bIsDown)
{
    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = ImGui_GetMouseButtonIndex(Button);
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