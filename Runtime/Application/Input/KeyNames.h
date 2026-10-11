#pragma once
#include "Core/Core.h"

struct EKeyName
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
        GamepadRightThumb    = 134,
        GamepadLeftThumb     = 135,
        GamepadRightShoulder = 136,
        GamepadLeftShoulder  = 137,
        GamepadStart         = 138,
        GamepadBack          = 139,

        First = Space,
        Last  = GamepadBack,

        KeyBoardFirst = Space,
        KeyBoardLast = Menu,

        MouseButtonFirst = MouseButtonLeft,
        MouseButtonLast = MouseButtonThumb2,

        GamepadFirst = GamepadDPadUp,
        GamepadLast = GamepadBack,

        Count = Last + 1
    };
};

constexpr const CHAR_T* ToString(EKeyName::Type KeyName)
{
    switch (KeyName)
    {
        // Numbers
        case EKeyName::Zero:  return TEXT("Zero");
        case EKeyName::One:   return TEXT("One");
        case EKeyName::Two:   return TEXT("Two");
        case EKeyName::Three: return TEXT("Three");
        case EKeyName::Four:  return TEXT("Four");
        case EKeyName::Five:  return TEXT("Five");
        case EKeyName::Six:   return TEXT("Six");
        case EKeyName::Seven: return TEXT("Seven");
        case EKeyName::Eight: return TEXT("Eight");
        case EKeyName::Nine:  return TEXT("Nine");

        // Letters
        case EKeyName::A: return TEXT("A");
        case EKeyName::B: return TEXT("B");
        case EKeyName::C: return TEXT("C");
        case EKeyName::D: return TEXT("D");
        case EKeyName::E: return TEXT("E");
        case EKeyName::F: return TEXT("F");
        case EKeyName::G: return TEXT("G");
        case EKeyName::H: return TEXT("H");
        case EKeyName::I: return TEXT("I");
        case EKeyName::J: return TEXT("J");
        case EKeyName::K: return TEXT("K");
        case EKeyName::L: return TEXT("L");
        case EKeyName::M: return TEXT("M");
        case EKeyName::N: return TEXT("N");
        case EKeyName::O: return TEXT("O");
        case EKeyName::P: return TEXT("P");
        case EKeyName::Q: return TEXT("Q");
        case EKeyName::R: return TEXT("R");
        case EKeyName::S: return TEXT("S");
        case EKeyName::T: return TEXT("T");
        case EKeyName::U: return TEXT("U");
        case EKeyName::V: return TEXT("V");
        case EKeyName::W: return TEXT("W");
        case EKeyName::X: return TEXT("X");
        case EKeyName::Y: return TEXT("Y");
        case EKeyName::Z: return TEXT("Z");

        // Function Keys
        case EKeyName::F1:  return TEXT("F1");
        case EKeyName::F2:  return TEXT("F2");
        case EKeyName::F3:  return TEXT("F3");
        case EKeyName::F4:  return TEXT("F4");
        case EKeyName::F5:  return TEXT("F5");
        case EKeyName::F6:  return TEXT("F6");
        case EKeyName::F7:  return TEXT("F7");
        case EKeyName::F8:  return TEXT("F8");
        case EKeyName::F9:  return TEXT("F9");
        case EKeyName::F10: return TEXT("F10");
        case EKeyName::F11: return TEXT("F11");
        case EKeyName::F12: return TEXT("F12");
        case EKeyName::F13: return TEXT("F13");
        case EKeyName::F14: return TEXT("F14");
        case EKeyName::F15: return TEXT("F15");
        case EKeyName::F16: return TEXT("F16");
        case EKeyName::F17: return TEXT("F17");
        case EKeyName::F18: return TEXT("F18");
        case EKeyName::F19: return TEXT("F19");
        case EKeyName::F20: return TEXT("F20");
        case EKeyName::F21: return TEXT("F21");
        case EKeyName::F22: return TEXT("F22");
        case EKeyName::F23: return TEXT("F23");
        case EKeyName::F24: return TEXT("F24");
        case EKeyName::F25: return TEXT("F25");

        // Keypad
        case EKeyName::KeypadZero:     return TEXT("KeypadZero");
        case EKeyName::KeypadOne:      return TEXT("KeypadOne");
        case EKeyName::KeypadTwo:      return TEXT("KeypadTwo");
        case EKeyName::KeypadThree:    return TEXT("KeypadThree");
        case EKeyName::KeypadFour:     return TEXT("KeypadFour");
        case EKeyName::KeypadFive:     return TEXT("KeypadFive");
        case EKeyName::KeypadSix:      return TEXT("KeypadSix");
        case EKeyName::KeypadSeven:    return TEXT("KeypadSeven");
        case EKeyName::KeypadEight:    return TEXT("KeypadEight");
        case EKeyName::KeypadNine:     return TEXT("KeypadNine");
        case EKeyName::KeypadDecimal:  return TEXT("KeypadDecimal");
        case EKeyName::KeypadDivide:   return TEXT("KeypadDivide");
        case EKeyName::KeypadMultiply: return TEXT("KeypadMultiply");
        case EKeyName::KeypadSubtract: return TEXT("KeypadSubtract");
        case EKeyName::KeypadAdd:      return TEXT("KeypadAdd");
        case EKeyName::KeypadEnter:    return TEXT("KeypadEnter");
        case EKeyName::KeypadEqual:    return TEXT("KeypadEqual");

        // Ctrl, Shift, Alt, etc.
        case EKeyName::LeftShift:    return TEXT("LeftShift");
        case EKeyName::LeftControl:  return TEXT("LeftControl");
        case EKeyName::LeftAlt:      return TEXT("LeftAlt");
        case EKeyName::LeftSuper:    return TEXT("LeftSuper");
        case EKeyName::RightShift:   return TEXT("RightShift");
        case EKeyName::RightControl: return TEXT("RightControl");
        case EKeyName::RightAlt:     return TEXT("RightAlt");
        case EKeyName::RightSuper:   return TEXT("RightSuper");
        case EKeyName::Menu:         return TEXT("Menu");

        // Other
        case EKeyName::Space:        return TEXT("Space");
        case EKeyName::Apostrophe:   return TEXT("Apostrophe");
        case EKeyName::Comma:        return TEXT("Comma");
        case EKeyName::Minus:        return TEXT("Minus");
        case EKeyName::Period:       return TEXT("Period");
        case EKeyName::Slash:        return TEXT("Slash");
        case EKeyName::Semicolon:    return TEXT("Semicolon");
        case EKeyName::Equal:        return TEXT("Equal");
        case EKeyName::LeftBracket:  return TEXT("LeftBracket");
        case EKeyName::Backslash:    return TEXT("Backslash");
        case EKeyName::RightBracket: return TEXT("RightBracket");
        case EKeyName::GraveAccent:  return TEXT("GraveAccent");
        case EKeyName::World1:       return TEXT("World1");
        case EKeyName::World2:       return TEXT("World2");
        case EKeyName::Escape:       return TEXT("Escape");
        case EKeyName::Enter:        return TEXT("Enter");
        case EKeyName::Tab:          return TEXT("Tab");
        case EKeyName::Backspace:    return TEXT("Backspace");
        case EKeyName::Insert:       return TEXT("Insert");
        case EKeyName::Delete:       return TEXT("Delete");
        case EKeyName::Right:        return TEXT("Right");
        case EKeyName::Left:         return TEXT("Left");
        case EKeyName::Down:         return TEXT("Down");
        case EKeyName::Up:           return TEXT("Up");
        case EKeyName::PageUp:       return TEXT("PageUp");
        case EKeyName::PageDown:     return TEXT("PageDown");
        case EKeyName::Home:         return TEXT("Home");
        case EKeyName::End:          return TEXT("End");
        case EKeyName::CapsLock:     return TEXT("CapsLock");
        case EKeyName::ScrollLock:   return TEXT("ScrollLock");
        case EKeyName::NumLock:      return TEXT("NumLock");
        case EKeyName::PrintScreen:  return TEXT("PrintScreen");
        case EKeyName::Pause:        return TEXT("Pause");

        // Mouse Buttons
        case EKeyName::MouseButtonLeft:   return TEXT("MouseButtonLeft");
        case EKeyName::MouseButtonRight:  return TEXT("MouseButtonRight");
        case EKeyName::MouseButtonMiddle: return TEXT("MouseButtonMiddle");
        case EKeyName::MouseButtonThumb1: return TEXT("MouseButtonThumb1");
        case EKeyName::MouseButtonThumb2: return TEXT("MouseButtonThumb2");

        // Gamepad Buttons
        case EKeyName::GamepadDPadUp:        return TEXT("GamepadDPadUp");
        case EKeyName::GamepadDPadDown:      return TEXT("GamepadDPadDown");
        case EKeyName::GamepadDPadLeft:      return TEXT("GamepadDPadLeft");
        case EKeyName::GamepadDPadRight:     return TEXT("GamepadDPadRight");
        case EKeyName::GamepadFaceUp:        return TEXT("GamepadFaceUp");
        case EKeyName::GamepadFaceDown:      return TEXT("GamepadFaceDown");
        case EKeyName::GamepadFaceLeft:      return TEXT("GamepadFaceLeft");
        case EKeyName::GamepadFaceRight:     return TEXT("GamepadFaceRight");
        case EKeyName::GamepadRightThumb:    return TEXT("GamepadRightThumb");
        case EKeyName::GamepadLeftThumb:     return TEXT("GamepadLeftThumb");
        case EKeyName::GamepadRightShoulder: return TEXT("GamepadRightShoulder");
        case EKeyName::GamepadLeftShoulder:  return TEXT("GamepadLeftShoulder");
        case EKeyName::GamepadStart:         return TEXT("GamepadStart");
        case EKeyName::GamepadBack:          return TEXT("GamepadBack");

        default: return TEXT("Unknown");
    }
}
