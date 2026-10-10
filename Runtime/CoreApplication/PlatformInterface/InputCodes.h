#pragma once
#include "Core/Core.h"

struct EKeyboardKeyName
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
};

constexpr const CHAR_T* ToString(EKeyboardKeyName::Type key)
{
    switch (key)
    {
    case EKeyboardKeyName::Zero:           return TEXT("0");
    case EKeyboardKeyName::One:            return TEXT("1");
    case EKeyboardKeyName::Two:            return TEXT("2");
    case EKeyboardKeyName::Three:          return TEXT("3");
    case EKeyboardKeyName::Four:           return TEXT("4");
    case EKeyboardKeyName::Five:           return TEXT("5");
    case EKeyboardKeyName::Six:            return TEXT("6");
    case EKeyboardKeyName::Seven:          return TEXT("7");
    case EKeyboardKeyName::Eight:          return TEXT("8");
    case EKeyboardKeyName::Nine:           return TEXT("9");
    case EKeyboardKeyName::A:              return TEXT("A");
    case EKeyboardKeyName::B:              return TEXT("B");
    case EKeyboardKeyName::C:              return TEXT("C");
    case EKeyboardKeyName::D:              return TEXT("D");
    case EKeyboardKeyName::E:              return TEXT("E");
    case EKeyboardKeyName::F:              return TEXT("F");
    case EKeyboardKeyName::G:              return TEXT("G");
    case EKeyboardKeyName::H:              return TEXT("H");
    case EKeyboardKeyName::I:              return TEXT("I");
    case EKeyboardKeyName::J:              return TEXT("J");
    case EKeyboardKeyName::K:              return TEXT("K");
    case EKeyboardKeyName::L:              return TEXT("L");
    case EKeyboardKeyName::M:              return TEXT("M");
    case EKeyboardKeyName::N:              return TEXT("N");
    case EKeyboardKeyName::O:              return TEXT("O");
    case EKeyboardKeyName::P:              return TEXT("P");
    case EKeyboardKeyName::Q:              return TEXT("Q");
    case EKeyboardKeyName::R:              return TEXT("R");
    case EKeyboardKeyName::S:              return TEXT("S");
    case EKeyboardKeyName::T:              return TEXT("T");
    case EKeyboardKeyName::U:              return TEXT("U");
    case EKeyboardKeyName::V:              return TEXT("V");
    case EKeyboardKeyName::W:              return TEXT("W");
    case EKeyboardKeyName::X:              return TEXT("X");
    case EKeyboardKeyName::Y:              return TEXT("Y");
    case EKeyboardKeyName::Z:              return TEXT("Z");
    case EKeyboardKeyName::F1:             return TEXT("F1");
    case EKeyboardKeyName::F2:             return TEXT("F2");
    case EKeyboardKeyName::F3:             return TEXT("F3");
    case EKeyboardKeyName::F4:             return TEXT("F4");
    case EKeyboardKeyName::F5:             return TEXT("F5");
    case EKeyboardKeyName::F6:             return TEXT("F6");
    case EKeyboardKeyName::F7:             return TEXT("F7");
    case EKeyboardKeyName::F8:             return TEXT("F8");
    case EKeyboardKeyName::F9:             return TEXT("F9");
    case EKeyboardKeyName::F10:            return TEXT("F10");
    case EKeyboardKeyName::F11:            return TEXT("F11");
    case EKeyboardKeyName::F12:            return TEXT("F12");
    case EKeyboardKeyName::F13:            return TEXT("F13");
    case EKeyboardKeyName::F14:            return TEXT("F14");
    case EKeyboardKeyName::F15:            return TEXT("F15");
    case EKeyboardKeyName::F16:            return TEXT("F16");
    case EKeyboardKeyName::F17:            return TEXT("F17");
    case EKeyboardKeyName::F18:            return TEXT("F18");
    case EKeyboardKeyName::F19:            return TEXT("F19");
    case EKeyboardKeyName::F20:            return TEXT("F20");
    case EKeyboardKeyName::F21:            return TEXT("F21");
    case EKeyboardKeyName::F22:            return TEXT("F22");
    case EKeyboardKeyName::F23:            return TEXT("F23");
    case EKeyboardKeyName::F24:            return TEXT("F24");
    case EKeyboardKeyName::F25:            return TEXT("F25");
    case EKeyboardKeyName::KeypadZero:     return TEXT("Keypad0");
    case EKeyboardKeyName::KeypadOne:      return TEXT("Keypad1");
    case EKeyboardKeyName::KeypadTwo:      return TEXT("Keypad2");
    case EKeyboardKeyName::KeypadThree:    return TEXT("Keypad3");
    case EKeyboardKeyName::KeypadFour:     return TEXT("Keypad4");
    case EKeyboardKeyName::KeypadFive:     return TEXT("Keypad5");
    case EKeyboardKeyName::KeypadSix:      return TEXT("Keypad6");
    case EKeyboardKeyName::KeypadSeven:    return TEXT("Keypad7");
    case EKeyboardKeyName::KeypadEight:    return TEXT("Keypad8");
    case EKeyboardKeyName::KeypadNine:     return TEXT("Keypad9");
    case EKeyboardKeyName::KeypadDecimal:  return TEXT("KeypadDecimal");
    case EKeyboardKeyName::KeypadDivide:   return TEXT("KeypadDivide");
    case EKeyboardKeyName::KeypadMultiply: return TEXT("KeypadMultiply");
    case EKeyboardKeyName::KeypadSubtract: return TEXT("KeypadSubtract");
    case EKeyboardKeyName::KeypadAdd:      return TEXT("KeypadAdd");
    case EKeyboardKeyName::KeypadEnter:    return TEXT("KeypadEnter");
    case EKeyboardKeyName::KeypadEqual:    return TEXT("KeypadEqual");
    case EKeyboardKeyName::LeftShift:      return TEXT("LeftShift");
    case EKeyboardKeyName::LeftControl:    return TEXT("LeftControl");
    case EKeyboardKeyName::LeftAlt:        return TEXT("LeftAlt");
    case EKeyboardKeyName::LeftSuper:      return TEXT("LeftSuper");
    case EKeyboardKeyName::RightShift:     return TEXT("RightShift");
    case EKeyboardKeyName::RightControl:   return TEXT("RightControl");
    case EKeyboardKeyName::RightAlt:       return TEXT("RightAlt");
    case EKeyboardKeyName::RightSuper:     return TEXT("RightSuper");
    case EKeyboardKeyName::Menu:           return TEXT("Menu");
    case EKeyboardKeyName::Space:          return TEXT("Space");
    case EKeyboardKeyName::Apostrophe:     return TEXT("Apostrophe");
    case EKeyboardKeyName::Comma:          return TEXT("Comma");
    case EKeyboardKeyName::Minus:          return TEXT("Minus");
    case EKeyboardKeyName::Period:         return TEXT("Period");
    case EKeyboardKeyName::Slash:          return TEXT("Slash");
    case EKeyboardKeyName::Semicolon:      return TEXT("Semicolon");
    case EKeyboardKeyName::Equal:          return TEXT("Equal");
    case EKeyboardKeyName::LeftBracket:    return TEXT("LeftBracket");
    case EKeyboardKeyName::Backslash:      return TEXT("Backslash");
    case EKeyboardKeyName::RightBracket:   return TEXT("RightBracket");
    case EKeyboardKeyName::GraveAccent:    return TEXT("GraveAccent");
    case EKeyboardKeyName::World1:         return TEXT("World1");
    case EKeyboardKeyName::World2:         return TEXT("World2");
    case EKeyboardKeyName::Escape:         return TEXT("Escape");
    case EKeyboardKeyName::Enter:          return TEXT("Enter");
    case EKeyboardKeyName::Tab:            return TEXT("Tab");
    case EKeyboardKeyName::Backspace:      return TEXT("Backspace");
    case EKeyboardKeyName::Insert:         return TEXT("Insert");
    case EKeyboardKeyName::Delete:         return TEXT("Delete");
    case EKeyboardKeyName::Right:          return TEXT("Right");
    case EKeyboardKeyName::Left:           return TEXT("Left");
    case EKeyboardKeyName::Down:           return TEXT("Down");
    case EKeyboardKeyName::Up:             return TEXT("Up");
    case EKeyboardKeyName::PageUp:         return TEXT("PageUp");
    case EKeyboardKeyName::PageDown:       return TEXT("PageDown");
    case EKeyboardKeyName::Home:           return TEXT("Home");
    case EKeyboardKeyName::End:            return TEXT("End");
    case EKeyboardKeyName::CapsLock:       return TEXT("CapsLock");
    case EKeyboardKeyName::ScrollLock:     return TEXT("ScrollLock");
    case EKeyboardKeyName::NumLock:        return TEXT("NumLock");
    case EKeyboardKeyName::PrintScreen:    return TEXT("PrintScreen");
    case EKeyboardKeyName::Pause:          return TEXT("Pause");
    default:                               return TEXT("Unknown");
    }
}

struct EMouseButtonName
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
};

constexpr const CHAR_T* ToString(EMouseButtonName::Type Button)
{
    switch (Button)
    {
    case EMouseButtonName::Left:   return TEXT("Left");
    case EMouseButtonName::Right:  return TEXT("Right");
    case EMouseButtonName::Middle: return TEXT("Middle");
    case EMouseButtonName::Thumb1: return TEXT("Thumb1");
    case EMouseButtonName::Thumb2: return TEXT("Thumb2");
    default:                       return TEXT("Unknown");
    }
}

enum class EScrollAxis : uint8
{
    Horizontal,
    Vertical,
};

constexpr const CHAR_T* ToString(EScrollAxis ScrollAxis)
{
    switch (ScrollAxis)
    {
    case EScrollAxis::Horizontal: return TEXT("Horizontal");
    case EScrollAxis::Vertical:   return TEXT("Vertical");
    default:                      return TEXT("Unknown");
    }
}

struct EGamepadButtonName
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
};

constexpr const CHAR_T* ToString(EGamepadButtonName::Type Button)
{
    switch (Button)
    {
    case EGamepadButtonName::DPadUp:        return TEXT("DPadUp");
    case EGamepadButtonName::DPadDown:      return TEXT("DPadDown");
    case EGamepadButtonName::DPadLeft:      return TEXT("DPadLeft");
    case EGamepadButtonName::DPadRight:     return TEXT("DPadRight");
    case EGamepadButtonName::FaceUp:        return TEXT("FaceUp");
    case EGamepadButtonName::FaceDown:      return TEXT("FaceDown");
    case EGamepadButtonName::FaceLeft:      return TEXT("FaceLeft");
    case EGamepadButtonName::FaceRight:     return TEXT("FaceRight");
    case EGamepadButtonName::RightThumb:    return TEXT("RightThumb");
    case EGamepadButtonName::LeftThumb:     return TEXT("LeftThumb");
    case EGamepadButtonName::RightShoulder: return TEXT("RightShoulder");
    case EGamepadButtonName::LeftShoulder:  return TEXT("LeftShoulder");
    case EGamepadButtonName::Start:         return TEXT("Start");
    case EGamepadButtonName::Back:          return TEXT("Back");
    default:                                return TEXT("Unknown");
    }
}

struct EAnalogSourceName
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
};

constexpr const CHAR_T* ToString(EAnalogSourceName::Type AnalogSourceName)
{
    switch (AnalogSourceName)
    {
    case EAnalogSourceName::RightThumbX:  return TEXT("RightThumbX");
    case EAnalogSourceName::RightThumbY:  return TEXT("RightThumbY");
    case EAnalogSourceName::LeftThumbX:   return TEXT("LeftThumbX");
    case EAnalogSourceName::LeftThumbY:   return TEXT("LeftThumbY");
    case EAnalogSourceName::RightTrigger: return TEXT("RightTrigger");
    case EAnalogSourceName::LeftTrigger:  return TEXT("LeftTrigger");
    default:                              return TEXT("Unknown");
    }
}
