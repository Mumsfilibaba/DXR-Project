#pragma once
#include "Core/Core.h"

namespace EKeyName
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
        Space        =  1,
        Apostrophe   =  2,  /* ' */
        Comma        =  3,  /* , */
        Minus        =  4,  /* - */
        Period       =  5,  /* . */
        Slash        =  6,  /* / */
        Semicolon    = 17,  /* ; */
        Equal        = 18,  /* = */
        LeftBracket  = 45,  /* [ */
        Backslash    = 46,  /* \ */
        RightBracket = 47,  /* ] */
        GraveAccent  = 48,  /* ` */
        World1       = 49,  /* Other */
        World2       = 50,  /* Other */
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

        // Mouse Buttons
        MouseButtonLeft   = 121,
        MouseButtonRight  = 122,
        MouseButtonMiddle = 123,
        MouseButtonThumb1 = 124,
        MouseButtonThumb2 = 125,

        // Gamepad Buttons
        GamepadDPadUp        = 126,
        GamepadDPadDown      = 127,
        GamepadDPadLeft      = 128,
        GamepadDPadRight     = 129,
        GamepadFaceUp        = 130,
        GamepadFaceDown      = 131,
        GamepadFaceLeft      = 132,
        GamepadFaceRight     = 133,
        GamepadRightTrigger  = 134,
        GamepadLeftTrigger   = 135,
        GamepadRightShoulder = 136,
        GamepadLeftShoulder  = 137,
        GamepadStart         = 138,
        GamepadBack          = 139,

        Last  = GamepadBack,
        Count = Last + 1
    };
}

constexpr const CHAR* ToString(EKeyName::Type KeyName)
{
    switch (KeyName)
    {
        // Numbers
        case EKeyName::Zero:  return "Zero";
        case EKeyName::One:   return "One";
        case EKeyName::Two:   return "Two";
        case EKeyName::Three: return "Three";
        case EKeyName::Four:  return "Four";
        case EKeyName::Five:  return "Five";
        case EKeyName::Six:   return "Six";
        case EKeyName::Seven: return "Seven";
        case EKeyName::Eight: return "Eight";
        case EKeyName::Nine:  return "Nine";

        // Letters
        case EKeyName::A: return "A";
        case EKeyName::B: return "B";
        case EKeyName::C: return "C";
        case EKeyName::D: return "D";
        case EKeyName::E: return "E";
        case EKeyName::F: return "F";
        case EKeyName::G: return "G";
        case EKeyName::H: return "H";
        case EKeyName::I: return "I";
        case EKeyName::J: return "J";
        case EKeyName::K: return "K";
        case EKeyName::L: return "L";
        case EKeyName::M: return "M";
        case EKeyName::N: return "N";
        case EKeyName::O: return "O";
        case EKeyName::P: return "P";
        case EKeyName::Q: return "Q";
        case EKeyName::R: return "R";
        case EKeyName::S: return "S";
        case EKeyName::T: return "T";
        case EKeyName::U: return "U";
        case EKeyName::V: return "V";
        case EKeyName::W: return "W";
        case EKeyName::X: return "X";
        case EKeyName::Y: return "Y";
        case EKeyName::Z: return "Z";

        // Function Keys
        case EKeyName::F1:  return "F1";
        case EKeyName::F2:  return "F2";
        case EKeyName::F3:  return "F3";
        case EKeyName::F4:  return "F4";
        case EKeyName::F5:  return "F5";
        case EKeyName::F6:  return "F6";
        case EKeyName::F7:  return "F7";
        case EKeyName::F8:  return "F8";
        case EKeyName::F9:  return "F9";
        case EKeyName::F10: return "F10";
        case EKeyName::F11: return "F11";
        case EKeyName::F12: return "F12";
        case EKeyName::F13: return "F13";
        case EKeyName::F14: return "F14";
        case EKeyName::F15: return "F15";
        case EKeyName::F16: return "F16";
        case EKeyName::F17: return "F17";
        case EKeyName::F18: return "F18";
        case EKeyName::F19: return "F19";
        case EKeyName::F20: return "F20";
        case EKeyName::F21: return "F21";
        case EKeyName::F22: return "F22";
        case EKeyName::F23: return "F23";
        case EKeyName::F24: return "F24";
        case EKeyName::F25: return "F25";

        // Keypad
        case EKeyName::KeypadZero:     return "KeypadZero";
        case EKeyName::KeypadOne:      return "KeypadOne";
        case EKeyName::KeypadTwo:      return "KeypadTwo";
        case EKeyName::KeypadThree:    return "KeypadThree";
        case EKeyName::KeypadFour:     return "KeypadFour";
        case EKeyName::KeypadFive:     return "KeypadFive";
        case EKeyName::KeypadSix:      return "KeypadSix";
        case EKeyName::KeypadSeven:    return "KeypadSeven";
        case EKeyName::KeypadEight:    return "KeypadEight";
        case EKeyName::KeypadNine:     return "KeypadNine";
        case EKeyName::KeypadDecimal:  return "KeypadDecimal";
        case EKeyName::KeypadDivide:   return "KeypadDivide";
        case EKeyName::KeypadMultiply: return "KeypadMultiply";
        case EKeyName::KeypadSubtract: return "KeypadSubtract";
        case EKeyName::KeypadAdd:      return "KeypadAdd";
        case EKeyName::KeypadEnter:    return "KeypadEnter";
        case EKeyName::KeypadEqual:    return "KeypadEqual";

        // Ctrl, Shift, Alt, etc.
        case EKeyName::LeftShift:    return "LeftShift";
        case EKeyName::LeftControl:  return "LeftControl";
        case EKeyName::LeftAlt:      return "LeftAlt";
        case EKeyName::LeftSuper:    return "LeftSuper";
        case EKeyName::RightShift:   return "RightShift";
        case EKeyName::RightControl: return "RightControl";
        case EKeyName::RightAlt:     return "RightAlt";
        case EKeyName::RightSuper:   return "RightSuper";
        case EKeyName::Menu:         return "Menu";

        // Other
        case EKeyName::Space:        return "Space";
        case EKeyName::Apostrophe:   return "Apostrophe";
        case EKeyName::Comma:        return "Comma";
        case EKeyName::Minus:        return "Minus";
        case EKeyName::Period:       return "Period";
        case EKeyName::Slash:        return "Slash";
        case EKeyName::Semicolon:    return "Semicolon";
        case EKeyName::Equal:        return "Equal";
        case EKeyName::LeftBracket:  return "LeftBracket";
        case EKeyName::Backslash:    return "Backslash";
        case EKeyName::RightBracket: return "RightBracket";
        case EKeyName::GraveAccent:  return "GraveAccent";
        case EKeyName::World1:       return "World1";
        case EKeyName::World2:       return "World2";
        case EKeyName::Escape:       return "Escape";
        case EKeyName::Enter:        return "Enter";
        case EKeyName::Tab:          return "Tab";
        case EKeyName::Backspace:    return "Backspace";
        case EKeyName::Insert:       return "Insert";
        case EKeyName::Delete:       return "Delete";
        case EKeyName::Right:        return "Right";
        case EKeyName::Left:         return "Left";
        case EKeyName::Down:         return "Down";
        case EKeyName::Up:           return "Up";
        case EKeyName::PageUp:       return "PageUp";
        case EKeyName::PageDown:     return "PageDown";
        case EKeyName::Home:         return "Home";
        case EKeyName::End:          return "End";
        case EKeyName::CapsLock:     return "CapsLock";
        case EKeyName::ScrollLock:   return "ScrollLock";
        case EKeyName::NumLock:      return "NumLock";
        case EKeyName::PrintScreen:  return "PrintScreen";
        case EKeyName::Pause:        return "Pause";

        // Mouse Buttons
        case EKeyName::MouseButtonLeft:   return "MouseButtonLeft";
        case EKeyName::MouseButtonRight:  return "MouseButtonRight";
        case EKeyName::MouseButtonMiddle: return "MouseButtonMiddle";
        case EKeyName::MouseButtonThumb1: return "MouseButtonThumb1";
        case EKeyName::MouseButtonThumb2: return "MouseButtonThumb2";

        // Gamepad Buttons
        case EKeyName::GamepadDPadUp:        return "GamepadDPadUp";
        case EKeyName::GamepadDPadDown:      return "GamepadDPadDown";
        case EKeyName::GamepadDPadLeft:      return "GamepadDPadLeft";
        case EKeyName::GamepadDPadRight:     return "GamepadDPadRight";
        case EKeyName::GamepadFaceUp:        return "GamepadFaceUp";
        case EKeyName::GamepadFaceDown:      return "GamepadFaceDown";
        case EKeyName::GamepadFaceLeft:      return "GamepadFaceLeft";
        case EKeyName::GamepadFaceRight:     return "GamepadFaceRight";
        case EKeyName::GamepadRightTrigger:  return "GamepadRightTrigger";
        case EKeyName::GamepadLeftTrigger:   return "GamepadLeftTrigger";
        case EKeyName::GamepadRightShoulder: return "GamepadRightShoulder";
        case EKeyName::GamepadLeftShoulder:  return "GamepadLeftShoulder";
        case EKeyName::GamepadStart:         return "GamepadStart";
        case EKeyName::GamepadBack:          return "GamepadBack";

        default:
            return "Unknown";
    }
}
