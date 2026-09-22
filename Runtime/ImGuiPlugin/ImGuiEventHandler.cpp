#include "ImGuiPlugin.h"
#include "ImGuiExtensions.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Application/Application.h"
#include "Application/Input/InputMapper.h"

#define IMGUI_BUTTON_UNKNOWN (-1)
#define IMGUI_BUTTON_THUMB1 (3)
#define IMGUI_BUTTON_THUMB2 (4)

static bool HasApplicationMouseCapture()
{
    return FApplication::IsInitialized() && FApplication::Get().HasMouseCapture();
}

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
    /* EGamepadButtonName::RightThumb */    ImGuiKey_GamepadR3,
    /* EGamepadButtonName::LeftThumb */     ImGuiKey_GamepadL3,
    /* EGamepadButtonName::RightShoulder */ ImGuiKey_GamepadR1,
    /* EGamepadButtonName::LeftShoulder */  ImGuiKey_GamepadL1,
    /* EGamepadButtonName::Start */         ImGuiKey_GamepadStart,
    /* EGamepadButtonName::Back */          ImGuiKey_GamepadBack,
};

static FORCEINLINE ImGuiKey GetImGuiGamepadButton(EGamepadButtonName::Type Button)
{
    CHECK(Button >= EGamepadButtonName::First && Button <= EGamepadButtonName::Last);
    return GImGuiGamepadKeys[Button];
}

static FORCEINLINE void SetImGuiStickAxis(ImGuiIO& UIState, ImGuiKey PositiveKey, ImGuiKey NegativeKey, float Value)
{
    const float Magnitude = Math::Abs(Value);
    const bool  bActive   = Magnitude > 0.10f;

    if (Value > 0.0f)
    {
        UIState.AddKeyAnalogEvent(PositiveKey, bActive, Magnitude);
        UIState.AddKeyAnalogEvent(NegativeKey, false, 0.0f);
    }
    else if (Value < 0.0f)
    {
        UIState.AddKeyAnalogEvent(NegativeKey, bActive, Magnitude);
        UIState.AddKeyAnalogEvent(PositiveKey, false, 0.0f);
    }
    else
    {
        UIState.AddKeyAnalogEvent(PositiveKey, false, 0.0f);
        UIState.AddKeyAnalogEvent(NegativeKey, false, 0.0f);
    }
}

bool FImGuiEventHandler::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent)
{
    ImGuiIO& UIState = ImGui::GetIO();
    const float Value = AnalogEvent.GetAnalogValue();

    switch (AnalogEvent.GetAnalogSource())
    {
        case EAnalogSourceName::LeftThumbX:
        {
            SetImGuiStickAxis(UIState, ImGuiKey_GamepadLStickRight, ImGuiKey_GamepadLStickLeft, Value);
            break;
        }

        case EAnalogSourceName::LeftThumbY:
        {
            SetImGuiStickAxis(UIState, ImGuiKey_GamepadLStickUp, ImGuiKey_GamepadLStickDown, Value);
            break;
        }

        case EAnalogSourceName::RightThumbX:
        {
            SetImGuiStickAxis(UIState, ImGuiKey_GamepadRStickRight, ImGuiKey_GamepadRStickLeft, Value);
            break;
        }

        case EAnalogSourceName::RightThumbY:
        {
            SetImGuiStickAxis(UIState, ImGuiKey_GamepadRStickUp, ImGuiKey_GamepadRStickDown, Value);
            break;
        }

        case EAnalogSourceName::LeftTrigger:
        {
            const float Magnitude = Math::Clamp(Value, 0.0f, 1.0f);
            UIState.AddKeyAnalogEvent(ImGuiKey_GamepadL2, Magnitude > 0.10f, Magnitude);
            break;
        }

        case EAnalogSourceName::RightTrigger:
        {
            const float Magnitude = Math::Clamp(Value, 0.0f, 1.0f);
            UIState.AddKeyAnalogEvent(ImGuiKey_GamepadR2, Magnitude > 0.10f, Magnitude);
            break;
        }

        default:
        {
            break;
        }
    }

    return false;
}

void FImGuiEventHandler::ClearGamepadAnalogState()
{
    ImGuiIO& UIState = ImGui::GetIO();

    const ImGuiKey StickKeys[] =
    {
        ImGuiKey_GamepadLStickLeft,
        ImGuiKey_GamepadLStickRight,
        ImGuiKey_GamepadLStickUp,
        ImGuiKey_GamepadLStickDown,
        ImGuiKey_GamepadRStickLeft,
        ImGuiKey_GamepadRStickRight,
        ImGuiKey_GamepadRStickUp,
        ImGuiKey_GamepadRStickDown,
        ImGuiKey_GamepadL2,
        ImGuiKey_GamepadR2,
    };

    for (ImGuiKey Key : StickKeys)
    {
        UIState.AddKeyAnalogEvent(Key, false, 0.0f);
    }
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

        // NOTE: ImGuiKey_GraveAccent is the key that activates the console so we need to skip it so that we can disable the console.
        const ImGuiKey TranslatedKey = GetImGuiKeyboardKey(KeyName);
        if (TranslatedKey != ImGuiKey_GraveAccent && TranslatedKey != ImGuiKey_None)
        {
            UIState.AddKeyEvent(TranslatedKey, KeyEvent.IsDown());
            if (UIState.WantCaptureKeyboard && !HasApplicationMouseCapture())
            {
                return true;
            }
        }
    }

    return false;
}

bool FImGuiEventHandler::ProcessMouseButtonEvent(const FCursorEvent& CursorEvent)
{
    if (HasApplicationMouseCapture())
    {
        return false;
    }

    const EMouseButtonName::Type ButtonName = FInputMapper::Get().GetMouseButtonNameFromKey(CursorEvent.GetKey());
    CHECK(ButtonName != EMouseButtonName::Unknown);

    const ImGuiMouseButton ButtonIndex = GetImGuiMouseButton(ButtonName);
    CHECK(ButtonIndex >= 0);

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddMouseButtonEvent(ButtonIndex, CursorEvent.IsDown());
    return UIState.WantCaptureMouse;
}

bool FImGuiEventHandler::OnKeyChar(const FKeyEvent& KeyTypedEvent)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(KeyTypedEvent.GetAnsiChar());
    return false;
}

bool FImGuiEventHandler::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (HasApplicationMouseCapture())
    {
        return false;
    }

#ifndef EDITOR_BUILD
    const IntVector2 CursorPos = CursorEvent.GetClientPosition();
#else
    const IntVector2 CursorPos = CursorEvent.GetScreenPosition();
#endif

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
    if (HasApplicationMouseCapture())
    {
        return false;
    }

    ImGuiIO& UIState = ImGui::GetIO();
    if (CursorEvent.GetScrollAxis() == EScrollAxis::Vertical)
    {
        UIState.AddMouseWheelEvent(0.0f, CursorEvent.GetScrollDelta());
    }
    else
    {
        UIState.AddMouseWheelEvent(CursorEvent.GetScrollDelta(), 0.0f);
    }

    return false;
}
