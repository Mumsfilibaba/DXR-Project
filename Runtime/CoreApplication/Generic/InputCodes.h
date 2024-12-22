#pragma once
#include "Core/Core.h"

namespace EKeyboardKeyName
{
    enum Type : uint8
    {
        Unknown = 0,

        // Numbers
        Zero  = 7,
        One   = 8,
        Two   = 9,
        Three = 10,
        Four  = 11,
        Five  = 12,
        Six   = 13,
        Seven = 14,
        Eight = 15,
        Nine  = 16,

        // Letters
        A = 19,
        B = 20,
        C = 21,
        D = 22,
        E = 23,
        F = 24,
        G = 25,
        H = 26,
        I = 27,
        J = 28,
        K = 29,
        L = 30,
        M = 31,
        N = 32,
        O = 33,
        P = 34,
        Q = 35,
        R = 36,
        S = 37,
        T = 38,
        U = 39,
        V = 40,
        W = 41,
        X = 42,
        Y = 43,
        Z = 44,

        // Function Keys
        F1  = 70,
        F2  = 71,
        F3  = 72,
        F4  = 73,
        F5  = 74,
        F6  = 75,
        F7  = 76,
        F8  = 77,
        F9  = 78,
        F10 = 79,
        F11 = 80,
        F12 = 81,
        F13 = 82,
        F14 = 83,
        F15 = 84,
        F16 = 85,
        F17 = 86,
        F18 = 87,
        F19 = 88,
        F20 = 89,
        F21 = 90,
        F22 = 91,
        F23 = 92,
        F24 = 93,
        F25 = 94,

        // Keypad
        KeypadZero     = 95,
        KeypadOne      = 96,
        KeypadTwo      = 97,
        KeypadThree    = 98,
        KeypadFour     = 99,
        KeypadFive     = 100,
        KeypadSix      = 101,
        KeypadSeven    = 102,
        KeypadEight    = 103,
        KeypadNine     = 104,
        KeypadDecimal  = 105,
        KeypadDivide   = 106,
        KeypadMultiply = 107,
        KeypadSubtract = 108,
        KeypadAdd      = 109,
        KeypadEnter    = 110,
        KeypadEqual    = 111,

        // Ctrl, Shift, Alt, Etc.
        LeftShift    = 112,
        LeftControl  = 113,
        LeftAlt      = 114,
        LeftSuper    = 115,
        RightShift   = 116,
        RightControl = 117,
        RightAlt     = 118,
        RightSuper   = 119,
        Menu         = 120,

        // Other
        Space        = 1,
        Apostrophe   = 2,  /* ' */
        Comma        = 3,  /* , */
        Minus        = 4,  /* - */
        Period       = 5,  /* . */
        Slash        = 6,  /* / */
        Semicolon    = 17, /* ; */
        Equal        = 18, /* = */
        LeftBracket  = 45, /* [ */
        Backslash    = 46, /* \ */
        RightBracket = 47, /* ] */
        GraveAccent  = 48, /* ` */
        World1       = 49, /* Other */
        World2       = 50, /* Other */
        Escape       = 51,
        Enter        = 52,
        Tab          = 53,
        Backspace    = 54,
        Insert       = 55,
        Delete       = 56,
        Right        = 57,
        Left         = 58,
        Down         = 59,
        Up           = 60,
        PageUp       = 61,
        PageDown     = 62,
        Home         = 63,
        End          = 64,
        CapsLock     = 65,
        ScrollLock   = 66,
        NumLock      = 67,
        PrintScreen  = 68,
        Pause        = 69,

        First = Space,
        Last  = Menu,
        Count = Last + 1
    };
} 

constexpr const CHAR* ToString(EKeyboardKeyName::Type key)
{
    switch (key)
    {
    case EKeyboardKeyName::Zero:           return "0";
    case EKeyboardKeyName::One:            return "1";
    case EKeyboardKeyName::Two:            return "2";
    case EKeyboardKeyName::Three:          return "3";
    case EKeyboardKeyName::Four:           return "4";
    case EKeyboardKeyName::Five:           return "5";
    case EKeyboardKeyName::Six:            return "6";
    case EKeyboardKeyName::Seven:          return "7";
    case EKeyboardKeyName::Eight:          return "8";
    case EKeyboardKeyName::Nine:           return "9";
    case EKeyboardKeyName::A:              return "A";
    case EKeyboardKeyName::B:              return "B";
    case EKeyboardKeyName::C:              return "C";
    case EKeyboardKeyName::D:              return "D";
    case EKeyboardKeyName::E:              return "E";
    case EKeyboardKeyName::F:              return "F";
    case EKeyboardKeyName::G:              return "G";
    case EKeyboardKeyName::H:              return "H";
    case EKeyboardKeyName::I:              return "I";
    case EKeyboardKeyName::J:              return "J";
    case EKeyboardKeyName::K:              return "K";
    case EKeyboardKeyName::L:              return "L";
    case EKeyboardKeyName::M:              return "M";
    case EKeyboardKeyName::N:              return "N";
    case EKeyboardKeyName::O:              return "O";
    case EKeyboardKeyName::P:              return "P";
    case EKeyboardKeyName::Q:              return "Q";
    case EKeyboardKeyName::R:              return "R";
    case EKeyboardKeyName::S:              return "S";
    case EKeyboardKeyName::T:              return "T";
    case EKeyboardKeyName::U:              return "U";
    case EKeyboardKeyName::V:              return "V";
    case EKeyboardKeyName::W:              return "W";
    case EKeyboardKeyName::X:              return "X";
    case EKeyboardKeyName::Y:              return "Y";
    case EKeyboardKeyName::Z:              return "Z";
    case EKeyboardKeyName::F1:             return "F1";
    case EKeyboardKeyName::F2:             return "F2";
    case EKeyboardKeyName::F3:             return "F3";
    case EKeyboardKeyName::F4:             return "F4";
    case EKeyboardKeyName::F5:             return "F5";
    case EKeyboardKeyName::F6:             return "F6";
    case EKeyboardKeyName::F7:             return "F7";
    case EKeyboardKeyName::F8:             return "F8";
    case EKeyboardKeyName::F9:             return "F9";
    case EKeyboardKeyName::F10:            return "F10";
    case EKeyboardKeyName::F11:            return "F11";
    case EKeyboardKeyName::F12:            return "F12";
    case EKeyboardKeyName::F13:            return "F13";
    case EKeyboardKeyName::F14:            return "F14";
    case EKeyboardKeyName::F15:            return "F15";
    case EKeyboardKeyName::F16:            return "F16";
    case EKeyboardKeyName::F17:            return "F17";
    case EKeyboardKeyName::F18:            return "F18";
    case EKeyboardKeyName::F19:            return "F19";
    case EKeyboardKeyName::F20:            return "F20";
    case EKeyboardKeyName::F21:            return "F21";
    case EKeyboardKeyName::F22:            return "F22";
    case EKeyboardKeyName::F23:            return "F23";
    case EKeyboardKeyName::F24:            return "F24";
    case EKeyboardKeyName::F25:            return "F25";
    case EKeyboardKeyName::KeypadZero:     return "Keypad0";
    case EKeyboardKeyName::KeypadOne:      return "Keypad1";
    case EKeyboardKeyName::KeypadTwo:      return "Keypad2";
    case EKeyboardKeyName::KeypadThree:    return "Keypad3";
    case EKeyboardKeyName::KeypadFour:     return "Keypad4";
    case EKeyboardKeyName::KeypadFive:     return "Keypad5";
    case EKeyboardKeyName::KeypadSix:      return "Keypad6";
    case EKeyboardKeyName::KeypadSeven:    return "Keypad7";
    case EKeyboardKeyName::KeypadEight:    return "Keypad8";
    case EKeyboardKeyName::KeypadNine:     return "Keypad9";
    case EKeyboardKeyName::KeypadDecimal:  return "KeypadDecimal";
    case EKeyboardKeyName::KeypadDivide:   return "KeypadDivide";
    case EKeyboardKeyName::KeypadMultiply: return "KeypadMultiply";
    case EKeyboardKeyName::KeypadSubtract: return "KeypadSubtract";
    case EKeyboardKeyName::KeypadAdd:      return "KeypadAdd";
    case EKeyboardKeyName::KeypadEnter:    return "KeypadEnter";
    case EKeyboardKeyName::KeypadEqual:    return "KeypadEqual";
    case EKeyboardKeyName::LeftShift:      return "LeftShift";
    case EKeyboardKeyName::LeftControl:    return "LeftControl";
    case EKeyboardKeyName::LeftAlt:        return "LeftAlt";
    case EKeyboardKeyName::LeftSuper:      return "LeftSuper";
    case EKeyboardKeyName::RightShift:     return "RightShift";
    case EKeyboardKeyName::RightControl:   return "RightControl";
    case EKeyboardKeyName::RightAlt:       return "RightAlt";
    case EKeyboardKeyName::RightSuper:     return "RightSuper";
    case EKeyboardKeyName::Menu:           return "Menu";
    case EKeyboardKeyName::Space:          return "Space";
    case EKeyboardKeyName::Apostrophe:     return "Apostrophe";
    case EKeyboardKeyName::Comma:          return "Comma";
    case EKeyboardKeyName::Minus:          return "Minus";
    case EKeyboardKeyName::Period:         return "Period";
    case EKeyboardKeyName::Slash:          return "Slash";
    case EKeyboardKeyName::Semicolon:      return "Semicolon";
    case EKeyboardKeyName::Equal:          return "Equal";
    case EKeyboardKeyName::LeftBracket:    return "LeftBracket";
    case EKeyboardKeyName::Backslash:      return "Backslash";
    case EKeyboardKeyName::RightBracket:   return "RightBracket";
    case EKeyboardKeyName::GraveAccent:    return "GraveAccent";
    case EKeyboardKeyName::World1:         return "World1";
    case EKeyboardKeyName::World2:         return "World2";
    case EKeyboardKeyName::Escape:         return "Escape";
    case EKeyboardKeyName::Enter:          return "Enter";
    case EKeyboardKeyName::Tab:            return "Tab";
    case EKeyboardKeyName::Backspace:      return "Backspace";
    case EKeyboardKeyName::Insert:         return "Insert";
    case EKeyboardKeyName::Delete:         return "Delete";
    case EKeyboardKeyName::Right:          return "Right";
    case EKeyboardKeyName::Left:           return "Left";
    case EKeyboardKeyName::Down:           return "Down";
    case EKeyboardKeyName::Up:             return "Up";
    case EKeyboardKeyName::PageUp:         return "PageUp";
    case EKeyboardKeyName::PageDown:       return "PageDown";
    case EKeyboardKeyName::Home:           return "Home";
    case EKeyboardKeyName::End:            return "End";
    case EKeyboardKeyName::CapsLock:       return "CapsLock";
    case EKeyboardKeyName::ScrollLock:     return "ScrollLock";
    case EKeyboardKeyName::NumLock:        return "NumLock";
    case EKeyboardKeyName::PrintScreen:    return "PrintScreen";
    case EKeyboardKeyName::Pause:          return "Pause";
    default:                               return "Unknown";
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

        First = Left,
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
        RightThumb,
        LeftThumb,
        RightShoulder,
        LeftShoulder,
        Start,
        Back,

        First = DPadUp,
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
    case EGamepadButtonName::RightThumb:    return "RightThumb";
    case EGamepadButtonName::LeftThumb:     return "LeftThumb";
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

        First = RightThumbX,
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
