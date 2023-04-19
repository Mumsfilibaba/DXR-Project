#pragma once
#include "Core/Core.h"

enum EKey : uint8
{
    Key_Unknown        = 0,

    // Numbers
    Key_0              = 7,
    Key_1              = 8,
    Key_2              = 9,
    Key_3              = 10,
    Key_4              = 11,
    Key_5              = 12,
    Key_6              = 13,
    Key_7              = 14,
    Key_8              = 15,
    Key_9              = 16,

    // Letters
    Key_A              = 19,
    Key_B              = 20,
    Key_C              = 21,
    Key_D              = 22,
    Key_E              = 23,
    Key_F              = 24,
    Key_G              = 25,
    Key_H              = 26,
    Key_I              = 27,
    Key_J              = 28,
    Key_K              = 29,
    Key_L              = 30,
    Key_M              = 31,
    Key_N              = 32,
    Key_O              = 33,
    Key_P              = 34,
    Key_Q              = 35,
    Key_R              = 36,
    Key_S              = 37,
    Key_T              = 38,
    Key_U              = 39,
    Key_V              = 40,
    Key_W              = 41,
    Key_X              = 42,
    Key_Y              = 43,
    Key_Z              = 44,

    // Function Keys
    Key_F1             = 70,
    Key_F2             = 71,
    Key_F3             = 72,
    Key_F4             = 73,
    Key_F5             = 74,
    Key_F6             = 75,
    Key_F7             = 76,
    Key_F8             = 77,
    Key_F9             = 78,
    Key_F10            = 79,
    Key_F11            = 80,
    Key_F12            = 81,
    Key_F13            = 82,
    Key_F14            = 83,
    Key_F15            = 84,
    Key_F16            = 85,
    Key_F17            = 86,
    Key_F18            = 87,
    Key_F19            = 88,
    Key_F20            = 89,
    Key_F21            = 90,
    Key_F22            = 91,
    Key_F23            = 92,
    Key_F24            = 93,
    Key_F25            = 94,

    // Keypad
    Key_Keypad0        = 95,
    Key_Keypad1        = 96,
    Key_Keypad2        = 97,
    Key_Keypad3        = 98,
    Key_Keypad4        = 99,
    Key_Keypad5        = 100,
    Key_Keypad6        = 101,
    Key_Keypad7        = 102,
    Key_Keypad8        = 103,
    Key_Keypad9        = 104,
    Key_KeypadDecimal  = 105,
    Key_KeypadDivide   = 106,
    Key_KeypadMultiply = 107,
    Key_KeypadSubtract = 108,
    Key_KeypadAdd      = 109,
    Key_KeypadEnter    = 110,
    Key_KeypadEqual    = 111,

    // Ctrl, Shift, Alt, Etc.
    Key_LeftShift      = 112,
    Key_LeftControl    = 113,
    Key_LeftAlt        = 114,
    Key_LeftSuper      = 115,
    Key_RightShift     = 116,
    Key_RightControl   = 117,
    Key_RightAlt       = 118,
    Key_RightSuper     = 119,
    Key_Menu           = 120,

    // Other
    Key_Space          = 1,
    Key_Apostrophe     = 2,  /* ' */
    Key_Comma          = 3,  /* , */
    Key_Minus          = 4,  /* - */
    Key_Period         = 5,  /* . */
    Key_Slash          = 6,  /* / */
    Key_Semicolon      = 17, /* ; */
    Key_Equal          = 18, /* = */
    Key_LeftBracket    = 45, /* [ */
    Key_Backslash      = 46, /* \ */
    Key_RightBracket   = 47, /* ] */
    Key_GraveAccent    = 48, /* ` */
    Key_World1         = 49, /* Other */
    Key_World2         = 50, /* Other */
    Key_Escape         = 51,
    Key_Enter          = 52,
    Key_Tab            = 53,
    Key_Backspace      = 54,
    Key_Insert         = 55,
    Key_Delete         = 56,
    Key_Right          = 57,
    Key_Left           = 58,
    Key_Down           = 59,
    Key_Up             = 60,
    Key_PageUp         = 61,
    Key_PageDown       = 62,
    Key_Home           = 63,
    Key_End            = 64,
    Key_CapsLock       = 65,
    Key_ScrollLock     = 66,
    Key_NumLock        = 67,
    Key_PrintScreen    = 68,
    Key_Pause          = 69,

    Key_Last  = Key_Menu,
    Key_Count = Key_Last + 1
};


enum EMouseButton : uint8
{
    MouseButton_Unknown = 0,
    MouseButton_Left,
    MouseButton_Right,
    MouseButton_Middle,
    MouseButton_Back,
    MouseButton_Forward,

    MouseButton_Last  = MouseButton_Back,
    MouseButton_Count = MouseButton_Last + 1
};


enum EModifierFlag
{
    ModifierFlag_None     = 0,
    ModifierFlag_Ctrl     = FLAG(1),
    ModifierFlag_Alt      = FLAG(2),
    ModifierFlag_Shift    = FLAG(3),
    ModifierFlag_CapsLock = FLAG(4),
    ModifierFlag_Super    = FLAG(5),
    ModifierFlag_NumLock  = FLAG(6),
};

enum class EControllerButton
{
    Unknown = 0,

    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,

    FaceUp,
    FaceDown,
    FaceLeft,
    FaceRight,

    RightTrigger,
    LeftTrigger,

    RightShoulder,
    LeftShoulder,

    Start,
    Back,
};

enum class EControllerAnalog
{
    Unknown = 0,
    
    RightThumbX,
    RightThumbY,
    LeftThumbX,
    LeftThumbY,

    RightTrigger,
    LeftTrigger,
};

CONSTEXPR const CHAR* ToString(EKey key)
{
    switch (key)
    {
    case Key_0:              return "0";
    case Key_1:              return "1";
    case Key_2:              return "2";
    case Key_3:              return "3";
    case Key_4:              return "4";
    case Key_5:              return "5";
    case Key_6:              return "6";
    case Key_7:              return "7";
    case Key_8:              return "8";
    case Key_9:              return "9";
    case Key_A:              return "A";
    case Key_B:              return "B";
    case Key_C:              return "C";
    case Key_D:              return "D";
    case Key_E:              return "E";
    case Key_F:              return "F";
    case Key_G:              return "G";
    case Key_H:              return "H";
    case Key_I:              return "I";
    case Key_J:              return "J";
    case Key_K:              return "K";
    case Key_L:              return "L";
    case Key_M:              return "M";
    case Key_N:              return "N";
    case Key_O:              return "O";
    case Key_P:              return "P";
    case Key_Q:              return "Q";
    case Key_R:              return "R";
    case Key_S:              return "S";
    case Key_T:              return "T";
    case Key_U:              return "U";
    case Key_V:              return "V";
    case Key_W:              return "W";
    case Key_X:              return "X";
    case Key_Y:              return "Y";
    case Key_Z:              return "Z";
    case Key_F1:             return "F1";
    case Key_F2:             return "F2";
    case Key_F3:             return "F3";
    case Key_F4:             return "F4";
    case Key_F5:             return "F5";
    case Key_F6:             return "F6";
    case Key_F7:             return "F7";
    case Key_F8:             return "F8";
    case Key_F9:             return "F9";
    case Key_F10:            return "F10";
    case Key_F11:            return "F11";
    case Key_F12:            return "F12";
    case Key_F13:            return "F13";
    case Key_F14:            return "F14";
    case Key_F15:            return "F15";
    case Key_F16:            return "F16";
    case Key_F17:            return "F17";
    case Key_F18:            return "F18";
    case Key_F19:            return "F19";
    case Key_F20:            return "F20";
    case Key_F21:            return "F21";
    case Key_F22:            return "F22";
    case Key_F23:            return "F23";
    case Key_F24:            return "F24";
    case Key_F25:            return "F25";
    case Key_Keypad0:        return "KEYPAD_0";
    case Key_Keypad1:        return "KEYPAD_1";
    case Key_Keypad2:        return "KEYPAD_2";
    case Key_Keypad3:        return "KEYPAD_3";
    case Key_Keypad4:        return "KEYPAD_4";
    case Key_Keypad5:        return "KEYPAD_5";
    case Key_Keypad6:        return "KEYPAD_6";
    case Key_Keypad7:        return "KEYPAD_7";
    case Key_Keypad8:        return "KEYPAD_8";
    case Key_Keypad9:        return "KEYPAD_9";
    case Key_KeypadDecimal:  return "KEYPAD_DECIMAL";
    case Key_KeypadDivide:   return "KEYPAD_DIVIDE";
    case Key_KeypadMultiply: return "KEYPAD_MULTIPLY";
    case Key_KeypadSubtract: return "KEYPAD_SUBTRACT";
    case Key_KeypadAdd:      return "KEYPAD_ADD";
    case Key_KeypadEnter:    return "KEYPAD_ENTER";
    case Key_KeypadEqual:    return "KEYPAD_EQUAL";
    case Key_LeftShift:      return "LEFT_SHIFT";
    case Key_LeftControl:    return "LEFT_CONTROL";
    case Key_LeftAlt:        return "LEFT_ALT";
    case Key_LeftSuper:      return "LEFT_SUPER";
    case Key_RightShift:     return "RIGHT_SHIFT";
    case Key_RightControl:   return "RIGHT_CONTROL";
    case Key_RightAlt:       return "RIGHT_ALT";
    case Key_RightSuper:     return "RIGHT_SUPER";
    case Key_Menu:           return "MENU";
    case Key_Space:          return "SPACE";
    case Key_Apostrophe:     return "APOSTROPHE";
    case Key_Comma:          return "COMMA";
    case Key_Minus:          return "MINUS";
    case Key_Period:         return "PERIOD";
    case Key_Slash:          return "SLASH";
    case Key_Semicolon:      return "SEMICOLON";
    case Key_Equal:          return "EQUAL";
    case Key_LeftBracket:    return "LEFT_BRACKET";
    case Key_Backslash:      return "BACKSLASH";
    case Key_RightBracket:   return "RIGHT_BRACKET";
    case Key_GraveAccent:    return "GRAVE_ACCENT";
    case Key_World1:         return "WORLD_1";
    case Key_World2:         return "WORLD_2";
    case Key_Escape:         return "ESCAPE";
    case Key_Enter:          return "ENTER";
    case Key_Tab:            return "TAB";
    case Key_Backspace:      return "BACKSPACE";
    case Key_Insert:         return "INSERT";
    case Key_Delete:         return "DELETE";
    case Key_Right:          return "RIGHT";
    case Key_Left:           return "LEFT";
    case Key_Down:           return "DOWN";
    case Key_Up:             return "UP";
    case Key_PageUp:         return "PAGE_UP";
    case Key_PageDown:       return "PAGE_DOWN";
    case Key_Home:           return "HOME";
    case Key_End:            return "END";
    case Key_CapsLock:       return "CAPS_LOCK";
    case Key_ScrollLock:     return "SCROLL_LOCK";
    case Key_NumLock:        return "NUM_LOCK";
    case Key_PrintScreen:    return "PRINT_SCREEN";
    case Key_Pause:          return "PAUSE";
    default:                 return "UNKNOWN";
    }
}

CONSTEXPR const CHAR* ToString(EMouseButton Button)
{
    switch (Button)
    {
    case MouseButton_Left:    return "LeftMouse";
    case MouseButton_Right:   return "RightMouse";
    case MouseButton_Middle:  return "MiddleMouse";
    case MouseButton_Back:    return "Back";
    case MouseButton_Forward: return "Forward";
    default:                  return "Unknown";
    }
}

CONSTEXPR const CHAR* ToString(EControllerButton Button)
{
    switch (Button)
    {
    case EControllerButton::DPadUp:        return "DPadUp";
    case EControllerButton::DPadDown:      return "DPadDown";
    case EControllerButton::DPadLeft:      return "DPadLeft";
    case EControllerButton::DPadRight:     return "DPadRight";

    case EControllerButton::FaceUp:        return "FaceUp";
    case EControllerButton::FaceDown:      return "FaceDown";
    case EControllerButton::FaceLeft:      return "FaceLeft";
    case EControllerButton::FaceRight:     return "FaceRight";

    case EControllerButton::RightTrigger:  return "RightTrigger";
    case EControllerButton::LeftTrigger:   return "LeftTrigger";

    case EControllerButton::RightShoulder: return "RightShoulder";
    case EControllerButton::LeftShoulder:  return "LeftShoulder";

    case EControllerButton::Start:         return "Start";
    case EControllerButton::Back:          return "Back";
    default:                               return "Unknown";
    }
}

CONSTEXPR const CHAR* ToString(EControllerAnalog Button)
{
    switch (Button)
    {
    case EControllerAnalog::RightThumbX:  return "RightThumbX";
    case EControllerAnalog::RightThumbY:  return "RightThumbY";
    case EControllerAnalog::LeftThumbX:   return "LeftThumbX";
    case EControllerAnalog::LeftThumbY:   return "LeftThumbY";

    case EControllerAnalog::RightTrigger: return "RightTrigger";
    case EControllerAnalog::LeftTrigger:  return "LeftTrigger";
    default:                              return "Unknown";
    }
}