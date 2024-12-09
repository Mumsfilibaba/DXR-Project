#include "ImGuiPlugin.h"
#include "ImGuiExtensions.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Application/Input/InputMapper.h"

#define IMGUI_BUTTON_UNKNOWN (-1)
#define IMGUI_BUTTON_THUMB1 (3)
#define IMGUI_BUTTON_THUMB2 (4)

static ImGuiMouseButton GImGuiMouseButtons[EMouseButtonName::Count] = 
{
    /* EMouseButtonName::Unknown */ IMGUI_BUTTON_UNKNOWN,
    /* EMouseButtonName::Left    */ ImGuiMouseButton_Left,
    /* EMouseButtonName::Right   */ ImGuiMouseButton_Right,
    /* EMouseButtonName::Middle  */ ImGuiMouseButton_Middle,
    /* EMouseButtonName::Thumb1  */ IMGUI_BUTTON_THUMB1,
    /* EMouseButtonName::Thumb2  */ IMGUI_BUTTON_THUMB2,
};

static FORCEINLINE ImGuiMouseButton GetImGuiMouseButton(EMouseButtonName::Type Button)
{
    CHECK(Button >= EMouseButtonName::First && Button <= EMouseButtonName::Last);
    return GImGuiMouseButtons[Button];
}

static ImGuiKey GImGuiKeyboardKeys[EKeyboardKeyName::Count] = 
{
    /* EKeyboardKeyName::Unknown */        ImGuiKey_None,
    /* EKeyboardKeyName::Space */          ImGuiKey_Space,
    /* EKeyboardKeyName::Apostrophe */     ImGuiKey_Apostrophe,
    /* EKeyboardKeyName::Comma */          ImGuiKey_Comma,
    /* EKeyboardKeyName::Minus */          ImGuiKey_Minus,
    /* EKeyboardKeyName::Period */         ImGuiKey_Period,
    /* EKeyboardKeyName::Slash */          ImGuiKey_Slash,
    /* EKeyboardKeyName::Zero */           ImGuiKey_0,
    /* EKeyboardKeyName::One */            ImGuiKey_1,
    /* EKeyboardKeyName::Two */            ImGuiKey_2,
    /* EKeyboardKeyName::Three */          ImGuiKey_3,
    /* EKeyboardKeyName::Four */           ImGuiKey_4,
    /* EKeyboardKeyName::Five */           ImGuiKey_5,
    /* EKeyboardKeyName::Six */            ImGuiKey_6,
    /* EKeyboardKeyName::Seven */          ImGuiKey_7,
    /* EKeyboardKeyName::Eight */          ImGuiKey_8,
    /* EKeyboardKeyName::Nine */           ImGuiKey_9,
    /* EKeyboardKeyName::Semicolon */      ImGuiKey_Semicolon,
    /* EKeyboardKeyName::Equal */          ImGuiKey_Equal,
    /* EKeyboardKeyName::A */              ImGuiKey_A,
    /* EKeyboardKeyName::B */              ImGuiKey_B,
    /* EKeyboardKeyName::C */              ImGuiKey_C,
    /* EKeyboardKeyName::D */              ImGuiKey_D,
    /* EKeyboardKeyName::E */              ImGuiKey_E,
    /* EKeyboardKeyName::F */              ImGuiKey_F,
    /* EKeyboardKeyName::G */              ImGuiKey_G,
    /* EKeyboardKeyName::H */              ImGuiKey_H,
    /* EKeyboardKeyName::I */              ImGuiKey_I,
    /* EKeyboardKeyName::J */              ImGuiKey_J,
    /* EKeyboardKeyName::K */              ImGuiKey_K,
    /* EKeyboardKeyName::L */              ImGuiKey_L,
    /* EKeyboardKeyName::M */              ImGuiKey_M,
    /* EKeyboardKeyName::N */              ImGuiKey_N,
    /* EKeyboardKeyName::O */              ImGuiKey_O,
    /* EKeyboardKeyName::P */              ImGuiKey_P,
    /* EKeyboardKeyName::Q */              ImGuiKey_Q,
    /* EKeyboardKeyName::R */              ImGuiKey_R,
    /* EKeyboardKeyName::S */              ImGuiKey_S,
    /* EKeyboardKeyName::T */              ImGuiKey_T,
    /* EKeyboardKeyName::U */              ImGuiKey_U,
    /* EKeyboardKeyName::V */              ImGuiKey_V,
    /* EKeyboardKeyName::W */              ImGuiKey_W,
    /* EKeyboardKeyName::X */              ImGuiKey_X,
    /* EKeyboardKeyName::Y */              ImGuiKey_Y,
    /* EKeyboardKeyName::Z */              ImGuiKey_Z,
    /* EKeyboardKeyName::LeftBracket */    ImGuiKey_LeftBracket,
    /* EKeyboardKeyName::Backslash */      ImGuiKey_Backslash,
    /* EKeyboardKeyName::RightBracket */   ImGuiKey_RightBracket,
    /* EKeyboardKeyName::GraveAccent */    ImGuiKey_GraveAccent,
    /* EKeyboardKeyName::World1 */         ImGuiKey_None,
    /* EKeyboardKeyName::World2 */         ImGuiKey_None,
    /* EKeyboardKeyName::Escape */         ImGuiKey_Escape,
    /* EKeyboardKeyName::Enter */          ImGuiKey_Enter,
    /* EKeyboardKeyName::Tab */            ImGuiKey_Tab,
    /* EKeyboardKeyName::Backspace */      ImGuiKey_Backspace,
    /* EKeyboardKeyName::Insert */         ImGuiKey_Insert,
    /* EKeyboardKeyName::Delete */         ImGuiKey_Delete,
    /* EKeyboardKeyName::Right */          ImGuiKey_RightArrow,
    /* EKeyboardKeyName::Left */           ImGuiKey_LeftArrow,
    /* EKeyboardKeyName::Down */           ImGuiKey_DownArrow,
    /* EKeyboardKeyName::Up */             ImGuiKey_UpArrow,
    /* EKeyboardKeyName::PageUp */         ImGuiKey_PageUp,
    /* EKeyboardKeyName::PageDown */       ImGuiKey_PageDown,
    /* EKeyboardKeyName::Home */           ImGuiKey_Home,
    /* EKeyboardKeyName::End */            ImGuiKey_End,
    /* EKeyboardKeyName::CapsLock */       ImGuiKey_CapsLock,
    /* EKeyboardKeyName::ScrollLock */     ImGuiKey_ScrollLock,
    /* EKeyboardKeyName::NumLock */        ImGuiKey_NumLock,
    /* EKeyboardKeyName::PrintScreen */    ImGuiKey_PrintScreen,
    /* EKeyboardKeyName::Pause */          ImGuiKey_Pause,
    /* EKeyboardKeyName::F1 */             ImGuiKey_F1,
    /* EKeyboardKeyName::F2 */             ImGuiKey_F2,
    /* EKeyboardKeyName::F3 */             ImGuiKey_F3,
    /* EKeyboardKeyName::F4 */             ImGuiKey_F4,
    /* EKeyboardKeyName::F5 */             ImGuiKey_F5,
    /* EKeyboardKeyName::F6 */             ImGuiKey_F6,
    /* EKeyboardKeyName::F7 */             ImGuiKey_F7,
    /* EKeyboardKeyName::F8 */             ImGuiKey_F8,
    /* EKeyboardKeyName::F9 */             ImGuiKey_F9,
    /* EKeyboardKeyName::F10 */            ImGuiKey_F10,
    /* EKeyboardKeyName::F11 */            ImGuiKey_F11,
    /* EKeyboardKeyName::F12 */            ImGuiKey_F12,
    /* EKeyboardKeyName::F13 */            ImGuiKey_None,
    /* EKeyboardKeyName::F14 */            ImGuiKey_None,
    /* EKeyboardKeyName::F15 */            ImGuiKey_None,
    /* EKeyboardKeyName::F16 */            ImGuiKey_None,
    /* EKeyboardKeyName::F17 */            ImGuiKey_None,
    /* EKeyboardKeyName::F18 */            ImGuiKey_None,
    /* EKeyboardKeyName::F19 */            ImGuiKey_None,
    /* EKeyboardKeyName::F20 */            ImGuiKey_None,
    /* EKeyboardKeyName::F21 */            ImGuiKey_None,
    /* EKeyboardKeyName::F22 */            ImGuiKey_None,
    /* EKeyboardKeyName::F23 */            ImGuiKey_None,
    /* EKeyboardKeyName::F24 */            ImGuiKey_None,
    /* EKeyboardKeyName::F25 */            ImGuiKey_None,
    /* EKeyboardKeyName::KeypadZero */     ImGuiKey_Keypad0,
    /* EKeyboardKeyName::KeypadOne */      ImGuiKey_Keypad1,
    /* EKeyboardKeyName::KeypadTwo */      ImGuiKey_Keypad2,
    /* EKeyboardKeyName::KeypadThree */    ImGuiKey_Keypad3,
    /* EKeyboardKeyName::KeypadFour */     ImGuiKey_Keypad4,
    /* EKeyboardKeyName::KeypadFive */     ImGuiKey_Keypad5,
    /* EKeyboardKeyName::KeypadSix */      ImGuiKey_Keypad6,
    /* EKeyboardKeyName::KeypadSeven */    ImGuiKey_Keypad7,
    /* EKeyboardKeyName::KeypadEight */    ImGuiKey_Keypad8,
    /* EKeyboardKeyName::KeypadNine */     ImGuiKey_Keypad9,
    /* EKeyboardKeyName::KeypadDecimal */  ImGuiKey_KeypadDecimal,
    /* EKeyboardKeyName::KeypadDivide */   ImGuiKey_KeypadDivide,
    /* EKeyboardKeyName::KeypadMultiply */ ImGuiKey_KeypadMultiply,
    /* EKeyboardKeyName::KeypadSubtract */ ImGuiKey_KeypadSubtract,
    /* EKeyboardKeyName::KeypadAdd */      ImGuiKey_KeypadAdd,
    /* EKeyboardKeyName::KeypadEnter */    ImGuiKey_KeypadEnter,
    /* EKeyboardKeyName::LeftShift */      ImGuiKey_LeftShift,
    /* EKeyboardKeyName::LeftControl */    ImGuiKey_LeftCtrl,
    /* EKeyboardKeyName::LeftAlt */        ImGuiKey_LeftAlt,
    /* EKeyboardKeyName::LeftSuper */      ImGuiKey_LeftSuper,
    /* EKeyboardKeyName::RightShift */     ImGuiKey_RightShift,
    /* EKeyboardKeyName::RightControl */   ImGuiKey_RightCtrl,
    /* EKeyboardKeyName::RightAlt */       ImGuiKey_RightAlt,
    /* EKeyboardKeyName::RightSuper */     ImGuiKey_RightSuper,
    /* EKeyboardKeyName::Menu */           ImGuiKey_Menu,
};

static FORCEINLINE ImGuiKey GetImGuiKeyboardKey(EKeyboardKeyName::Type Key)
{
    CHECK(Key >= EKeyboardKeyName::First && Key <= EKeyboardKeyName::Last);
    return GImGuiKeyboardKeys[Key];
}

static ImGuiKey GImGuiGamepadKeys[EGamepadButtonName::Count] =
{
    /* EGamepadButtonName::Unknown */       ImGuiKey_None,
    /* EGamepadButtonName::DPadUp */        ImGuiKey_GamepadDpadUp,
    /* EGamepadButtonName::DPadDown */      ImGuiKey_GamepadDpadDown,
    /* EGamepadButtonName::DPadLeft */      ImGuiKey_GamepadDpadLeft,
    /* EGamepadButtonName::DPadRight */     ImGuiKey_GamepadDpadRight,
    /* EGamepadButtonName::FaceUp */        ImGuiKey_GamepadFaceUp,
    /* EGamepadButtonName::FaceDown */      ImGuiKey_GamepadFaceDown,
    /* EGamepadButtonName::FaceLeft */      ImGuiKey_GamepadFaceLeft,
    /* EGamepadButtonName::FaceRight */     ImGuiKey_GamepadFaceRight,
    /* EGamepadButtonName::RightTrigger */  ImGuiKey_GamepadR3,
    /* EGamepadButtonName::LeftTrigger */   ImGuiKey_GamepadL3,
    /* EGamepadButtonName::RightShoulder */ ImGuiKey_GamepadR3,
    /* EGamepadButtonName::LeftShoulder */  ImGuiKey_GamepadL3,
    /* EGamepadButtonName::Start */         ImGuiKey_GamepadStart,
    /* EGamepadButtonName::Back */          ImGuiKey_GamepadBack,
};

static FORCEINLINE ImGuiKey GetImGuiGamepadButton(EGamepadButtonName::Type Button)
{
    CHECK(Button >= EGamepadButtonName::First && Button <= EGamepadButtonName::Last);
    return GImGuiGamepadKeys[Button];
}

static FORCEINLINE ImGuiKey GetImGuiGamepadAnalogSource(EAnalogSourceName::Type Analog, bool bIsNegative)
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

bool FImGuiEventHandler::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent)
{
    const bool bIsNegative = AnalogEvent.GetAnalogValue() < 0.0f;
    const ImGuiKey GamepadButton = GetImGuiGamepadAnalogSource(AnalogEvent.GetAnalogSource(), bIsNegative);
    if (GamepadButton != ImGuiKey_None)
    {
        const float Normalized = FMath::Abs<float>(AnalogEvent.GetAnalogSource());
        
        ImGuiIO& UIState = ImGui::GetIO();
        UIState.AddKeyAnalogEvent(GamepadButton, Normalized > 0.10f, Normalized);
    }

    return false;
}

bool FImGuiEventHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ProcessKeyEvent(KeyEvent);
}

bool FImGuiEventHandler::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ProcessKeyEvent(KeyEvent);
}

bool FImGuiEventHandler::ProcessKeyEvent(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();
    if (Key.IsGamepadButton())
    {
        const EGamepadButtonName::Type Button = FInputMapper::Get().GetGamepadButtonNameFromKey(Key);
        CHECK(Button != EGamepadButtonName::Unknown);

        const ImGuiKey GamepadButton = GetImGuiGamepadButton(Button);
        if (GamepadButton != ImGuiKey_None)
        {
            ImGuiIO& UIState = ImGui::GetIO();
            UIState.AddKeyEvent(GamepadButton, KeyEvent.IsDown());
        }
    }
    else if (Key.IsKeyboardKey())
    {
        const EKeyboardKeyName::Type KeyName = FInputMapper::Get().GetKeyboardKeyNameFromKey(Key);
        CHECK(KeyName != EKeyboardKeyName::Unknown);

        ImGuiIO& UIState = ImGui::GetIO();
        UIState.AddKeyEvent(ImGuiMod_Ctrl, KeyEvent.GetModifierKeys().IsCtrlDown());
        UIState.AddKeyEvent(ImGuiMod_Shift, KeyEvent.GetModifierKeys().IsShiftDown());
        UIState.AddKeyEvent(ImGuiMod_Alt, KeyEvent.GetModifierKeys().IsAltDown());
        UIState.AddKeyEvent(ImGuiMod_Super, KeyEvent.GetModifierKeys().IsSuperDown());

        const ImGuiKey TranslatedKey = GetImGuiKeyboardKey(KeyName);

        // NOTE: ImGuiKey_GraveAccent is the key that activates the console so we need to skip it so that we can disable the console.
        if (TranslatedKey != ImGuiKey_GraveAccent)
        {
            if (TranslatedKey != ImGuiKey_None)
            {
                UIState.AddKeyEvent(TranslatedKey, KeyEvent.IsDown());
            }

            if (UIState.WantCaptureKeyboard)
            {
                return true;
            }
        }
    }

    return false;
}

bool FImGuiEventHandler::ProcessMouseButtonEvent(const FCursorEvent& CursorEvent)
{
    const EMouseButtonName::Type ButtonName = FInputMapper::Get().GetMouseButtonNameFromKey(CursorEvent.GetKey());
    CHECK(ButtonName != EMouseButtonName::Unknown);

    const ImGuiMouseButton ButtonIndex = GetImGuiMouseButton(ButtonName);
    CHECK(ButtonIndex >= 0);

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddMouseButtonEvent(ButtonIndex, CursorEvent.IsDown());

    if (UIState.WantCaptureMouse)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool FImGuiEventHandler::OnKeyChar(const FKeyEvent& KeyTypedEvent)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(KeyTypedEvent.GetAnsiChar());
    return false;
}

bool FImGuiEventHandler::OnMouseMove(const FCursorEvent& CursorEvent)
{
    FIntVector2 CursorPos = CursorEvent.GetCursorPos();
    
    if (!ImGuiExtensions::IsMultiViewportEnabled())
    {
        if (TSharedRef<FGenericWindow> Window = FApplicationInterface::Get().GetPlatformApplication()->GetWindowUnderCursor())
        {
            FWindowShape WindowShape;
            Window->GetWindowShape(WindowShape);

            CursorPos.X = CursorPos.X - WindowShape.Position.X;
            CursorPos.Y = CursorPos.Y - WindowShape.Position.Y;
        }
        else
        {
            CursorPos.X = -TNumericLimits<int32>::Max();
            CursorPos.Y = -TNumericLimits<int32>::Max();
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddMousePosEvent(static_cast<float>(CursorPos.X), static_cast<float>(CursorPos.Y));
    return false;
}

bool FImGuiEventHandler::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    return ProcessMouseButtonEvent(CursorEvent);
}

bool FImGuiEventHandler::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    return ProcessMouseButtonEvent(CursorEvent);
}

bool FImGuiEventHandler::OnMouseScrolled(const FCursorEvent& CursorEvent)
{
    ImGuiIO& UIState = ImGui::GetIO();
    if (CursorEvent.IsScrollVertical())
    {
        UIState.AddMouseWheelEvent(0.0f, CursorEvent.GetScrollDelta());
    }
    else
    {
        UIState.AddMouseWheelEvent(CursorEvent.GetScrollDelta(), 0.0f);
    }

    return false;
}
