#pragma once
#include "Core/Core.h"

enum EModifierFlag : uint8
{
    ModifierFlag_None     = 0,
    ModifierFlag_Ctrl     = FLAG(1),
    ModifierFlag_Alt      = FLAG(2),
    ModifierFlag_Shift    = FLAG(3),
    ModifierFlag_CapsLock = FLAG(4),
    ModifierFlag_Super    = FLAG(5),
    ModifierFlag_NumLock  = FLAG(6),
};


namespace EKeyName
{
    enum Type : uint8
    {
        Unknown            = 0,

        // Numbers
        Zero               = 7,
        One                = 8,
        Two                = 9,
        Three              = 10,
        Four               = 11,
        Five               = 12,
        Six                = 13,
        Seven              = 14,
        Eight              = 15,
        Nine               = 16,

        // Letters
        A                  = 19,
        B                  = 20,
        C                  = 21,
        D                  = 22,
        E                  = 23,
        F                  = 24,
        G                  = 25,
        H                  = 26,
        I                  = 27,
        J                  = 28,
        K                  = 29,
        L                  = 30,
        M                  = 31,
        N                  = 32,
        O                  = 33,
        P                  = 34,
        Q                  = 35,
        R                  = 36,
        S                  = 37,
        T                  = 38,
        U                  = 39,
        V                  = 40,
        W                  = 41,
        X                  = 42,
        Y                  = 43,
        Z                  = 44,

        // Function Keys
        F1                 = 70,
        F2                 = 71,
        F3                 = 72,
        F4                 = 73,
        F5                 = 74,
        F6                 = 75,
        F7                 = 76,
        F8                 = 77,
        F9                 = 78,
        F10                = 79,
        F11                = 80,
        F12                = 81,
        F13                = 82,
        F14                = 83,
        F15                = 84,
        F16                = 85,
        F17                = 86,
        F18                = 87,
        F19                = 88,
        F20                = 89,
        F21                = 90,
        F22                = 91,
        F23                = 92,
        F24                = 93,
        F25                = 94,

        // Keypad
        KeypadZero         = 95,
        KeypadOne          = 96,
        KeypadTwo          = 97,
        KeypadThree        = 98,
        KeypadFour         = 99,
        KeypadFive         = 100,
        KeypadSix          = 101,
        KeypadSeven        = 102,
        KeypadEight        = 103,
        KeypadNine         = 104,
        KeypadDecimal      = 105,
        KeypadDivide       = 106,
        KeypadMultiply     = 107,
        KeypadSubtract     = 108,
        KeypadAdd          = 109,
        KeypadEnter        = 110,
        KeypadEqual        = 111,

        // Ctrl, Shift, Alt, Etc.
        LeftShift          = 112,
        LeftControl        = 113,
        LeftAlt            = 114,
        LeftSuper          = 115,
        RightShift         = 116,
        RightControl       = 117,
        RightAlt           = 118,
        RightSuper         = 119,
        Menu               = 120,

        // Other
        Space              = 1,
        Apostrophe         = 2,  /* ' */
        Comma              = 3,  /* , */
        Minus              = 4,  /* - */
        Period             = 5,  /* . */
        Slash              = 6,  /* / */
        Semicolon          = 17, /* ; */
        Equal              = 18, /* = */
        LeftBracket        = 45, /* [ */
        Backslash          = 46, /* \ */
        RightBracket       = 47, /* ] */
        GraveAccent        = 48, /* ` */
        World1             = 49, /* Other */
        World2             = 50, /* Other */
        Escape             = 51,
        Enter              = 52,
        Tab                = 53,
        Backspace          = 54,
        Insert             = 55,
        Delete             = 56,
        Right              = 57,
        Left               = 58,
        Down               = 59,
        Up                 = 60,
        PageUp             = 61,
        PageDown           = 62,
        Home               = 63,
        End                = 64,
        CapsLock           = 65,
        ScrollLock         = 66,
        NumLock            = 67,
        PrintScreen        = 68,
        Pause              = 69,

        Last  = Menu,
        Count = Last + 1
    };
} 

constexpr const CHAR* ToString(EKeyName::Type key)
{
    switch (key)
    {
    case EKeyName::Zero:           return "0";
    case EKeyName::One:            return "1";
    case EKeyName::Two:            return "2";
    case EKeyName::Three:          return "3";
    case EKeyName::Four:           return "4";
    case EKeyName::Five:           return "5";
    case EKeyName::Six:            return "6";
    case EKeyName::Seven:          return "7";
    case EKeyName::Eight:          return "8";
    case EKeyName::Nine:           return "9";
    case EKeyName::A:              return "A";
    case EKeyName::B:              return "B";
    case EKeyName::C:              return "C";
    case EKeyName::D:              return "D";
    case EKeyName::E:              return "E";
    case EKeyName::F:              return "F";
    case EKeyName::G:              return "G";
    case EKeyName::H:              return "H";
    case EKeyName::I:              return "I";
    case EKeyName::J:              return "J";
    case EKeyName::K:              return "K";
    case EKeyName::L:              return "L";
    case EKeyName::M:              return "M";
    case EKeyName::N:              return "N";
    case EKeyName::O:              return "O";
    case EKeyName::P:              return "P";
    case EKeyName::Q:              return "Q";
    case EKeyName::R:              return "R";
    case EKeyName::S:              return "S";
    case EKeyName::T:              return "T";
    case EKeyName::U:              return "U";
    case EKeyName::V:              return "V";
    case EKeyName::W:              return "W";
    case EKeyName::X:              return "X";
    case EKeyName::Y:              return "Y";
    case EKeyName::Z:              return "Z";
    case EKeyName::F1:             return "F1";
    case EKeyName::F2:             return "F2";
    case EKeyName::F3:             return "F3";
    case EKeyName::F4:             return "F4";
    case EKeyName::F5:             return "F5";
    case EKeyName::F6:             return "F6";
    case EKeyName::F7:             return "F7";
    case EKeyName::F8:             return "F8";
    case EKeyName::F9:             return "F9";
    case EKeyName::F10:            return "F10";
    case EKeyName::F11:            return "F11";
    case EKeyName::F12:            return "F12";
    case EKeyName::F13:            return "F13";
    case EKeyName::F14:            return "F14";
    case EKeyName::F15:            return "F15";
    case EKeyName::F16:            return "F16";
    case EKeyName::F17:            return "F17";
    case EKeyName::F18:            return "F18";
    case EKeyName::F19:            return "F19";
    case EKeyName::F20:            return "F20";
    case EKeyName::F21:            return "F21";
    case EKeyName::F22:            return "F22";
    case EKeyName::F23:            return "F23";
    case EKeyName::F24:            return "F24";
    case EKeyName::F25:            return "F25";
    case EKeyName::KeypadZero:     return "KEYPAD_0";
    case EKeyName::KeypadOne:      return "KEYPAD_1";
    case EKeyName::KeypadTwo:      return "KEYPAD_2";
    case EKeyName::KeypadThree:    return "KEYPAD_3";
    case EKeyName::KeypadFour:     return "KEYPAD_4";
    case EKeyName::KeypadFive:     return "KEYPAD_5";
    case EKeyName::KeypadSix:      return "KEYPAD_6";
    case EKeyName::KeypadSeven:    return "KEYPAD_7";
    case EKeyName::KeypadEight:    return "KEYPAD_8";
    case EKeyName::KeypadNine:     return "KEYPAD_9";
    case EKeyName::KeypadDecimal:  return "KEYPAD_DECIMAL";
    case EKeyName::KeypadDivide:   return "KEYPAD_DIVIDE";
    case EKeyName::KeypadMultiply: return "KEYPAD_MULTIPLY";
    case EKeyName::KeypadSubtract: return "KEYPAD_SUBTRACT";
    case EKeyName::KeypadAdd:      return "KEYPAD_ADD";
    case EKeyName::KeypadEnter:    return "KEYPAD_ENTER";
    case EKeyName::KeypadEqual:    return "KEYPAD_EQUAL";
    case EKeyName::LeftShift:      return "LEFT_SHIFT";
    case EKeyName::LeftControl:    return "LEFT_CONTROL";
    case EKeyName::LeftAlt:        return "LEFT_ALT";
    case EKeyName::LeftSuper:      return "LEFT_SUPER";
    case EKeyName::RightShift:     return "RIGHT_SHIFT";
    case EKeyName::RightControl:   return "RIGHT_CONTROL";
    case EKeyName::RightAlt:       return "RIGHT_ALT";
    case EKeyName::RightSuper:     return "RIGHT_SUPER";
    case EKeyName::Menu:           return "MENU";
    case EKeyName::Space:          return "SPACE";
    case EKeyName::Apostrophe:     return "APOSTROPHE";
    case EKeyName::Comma:          return "COMMA";
    case EKeyName::Minus:          return "MINUS";
    case EKeyName::Period:         return "PERIOD";
    case EKeyName::Slash:          return "SLASH";
    case EKeyName::Semicolon:      return "SEMICOLON";
    case EKeyName::Equal:          return "EQUAL";
    case EKeyName::LeftBracket:    return "LEFT_BRACKET";
    case EKeyName::Backslash:      return "BACKSLASH";
    case EKeyName::RightBracket:   return "RIGHT_BRACKET";
    case EKeyName::GraveAccent:    return "GRAVE_ACCENT";
    case EKeyName::World1:         return "WORLD_1";
    case EKeyName::World2:         return "WORLD_2";
    case EKeyName::Escape:         return "ESCAPE";
    case EKeyName::Enter:          return "ENTER";
    case EKeyName::Tab:            return "TAB";
    case EKeyName::Backspace:      return "BACKSPACE";
    case EKeyName::Insert:         return "INSERT";
    case EKeyName::Delete:         return "DELETE";
    case EKeyName::Right:          return "RIGHT";
    case EKeyName::Left:           return "LEFT";
    case EKeyName::Down:           return "DOWN";
    case EKeyName::Up:             return "UP";
    case EKeyName::PageUp:         return "PAGE_UP";
    case EKeyName::PageDown:       return "PAGE_DOWN";
    case EKeyName::Home:           return "HOME";
    case EKeyName::End:            return "END";
    case EKeyName::CapsLock:       return "CAPS_LOCK";
    case EKeyName::ScrollLock:     return "SCROLL_LOCK";
    case EKeyName::NumLock:        return "NUM_LOCK";
    case EKeyName::PrintScreen:    return "PRINT_SCREEN";
    case EKeyName::Pause:          return "PAUSE";
    default:                       return "UNKNOWN";
    }
}


namespace EMouseButtonName
{
    enum Type : uint8
    {
        Unknown = 0,
        Left,
        Right,
        Middle,
        Thumb1,
        Thumb2,

        Last  = Thumb2,
        Count = Last + 1
    };
} 

constexpr const CHAR* ToString(EMouseButtonName::Type Button)
{
    switch (Button)
    {
    case EMouseButtonName::Left:   return "Left";
    case EMouseButtonName::Right:  return "Right";
    case EMouseButtonName::Middle: return "Middle";
    case EMouseButtonName::Thumb1: return "Thumb1";
    case EMouseButtonName::Thumb2: return "Thumb2";
    default:                       return "Unknown";
    }
}

namespace EGamepadButtonName
{
    enum Type : uint8
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

        Last  = Back,
        Count = Last + 1
    };
}

constexpr const CHAR* ToString(EGamepadButtonName::Type Button)
{
    switch (Button)
    {
    case EGamepadButtonName::DPadUp:        return "DPadUp";
    case EGamepadButtonName::DPadDown:      return "DPadDown";
    case EGamepadButtonName::DPadLeft:      return "DPadLeft";
    case EGamepadButtonName::DPadRight:     return "DPadRight";

    case EGamepadButtonName::FaceUp:        return "FaceUp";
    case EGamepadButtonName::FaceDown:      return "FaceDown";
    case EGamepadButtonName::FaceLeft:      return "FaceLeft";
    case EGamepadButtonName::FaceRight:     return "FaceRight";

    case EGamepadButtonName::RightTrigger:  return "RightTrigger";
    case EGamepadButtonName::LeftTrigger:   return "LeftTrigger";

    case EGamepadButtonName::RightShoulder: return "RightShoulder";
    case EGamepadButtonName::LeftShoulder:  return "LeftShoulder";

    case EGamepadButtonName::Start:         return "Start";
    case EGamepadButtonName::Back:          return "Back";
    default:                                return "Unknown";
    }
}

namespace EAnalogSourceName
{
    enum Type : uint8
    {
        Unknown = 0,
        
        RightThumbX,
        RightThumbY,
        LeftThumbX,
        LeftThumbY,

        RightTrigger,
        LeftTrigger,

        Last  = LeftTrigger,
        Count = Last + 1
    };
}

constexpr const CHAR* ToString(EAnalogSourceName::Type AnalogSourceName)
{
    switch (AnalogSourceName)
    {
    case EAnalogSourceName::RightThumbX:  return "RightThumbX";
    case EAnalogSourceName::RightThumbY:  return "RightThumbY";
    case EAnalogSourceName::LeftThumbX:   return "LeftThumbX";
    case EAnalogSourceName::LeftThumbY:   return "LeftThumbY";

    case EAnalogSourceName::RightTrigger: return "RightTrigger";
    case EAnalogSourceName::LeftTrigger:  return "LeftTrigger";
    default:                              return "Unknown";
    }
}
