#pragma once
#include "Application/Input/KeyNames.h"

class FKey
{
public:
    FKey()
        : Key(EKeyName::Unknown)
        , KeyString(::ToString(EKeyName::Unknown))
    {
    }

    explicit FKey(EKeyName::Type InKey) 
        : Key(InKey)
        , KeyString(::ToString(InKey))
    {
    }

    bool IsKeyboardKey() const
    {
        return Key >= EKeyName::Zero && Key <= EKeyName::Pause;
    }

    bool IsMouseButton() const
    {
        return Key >= EKeyName::MouseButtonLeft && Key <= EKeyName::MouseButtonThumb2;
    }

    bool IsGamepadButton() const
    {
        return Key >= EKeyName::GamepadDPadUp && Key <= EKeyName::Last;
    }

    const CHAR* ToString() const
    {
        return KeyString;
    }

    bool operator==(const FKey& Other) const
    {
        return Key == Other.Key;
    }

    bool operator!=(const FKey& Other) const
    {
        return Key != Other.Key;
    }

    EKeyName::Type GetKeyName() const
    {
        return Key;
    }

private:
    EKeyName::Type Key;
    const CHAR*    KeyString;
};

struct EKeys
{
    // Keyboard keys
    
    /** @brief - Unknown key */
    static APPLICATION_API const FKey Unknown;

    // Numbers
    
    /** @brief - Zero key */
    static APPLICATION_API const FKey Zero;
    
    /** @brief - One key */
    static APPLICATION_API const FKey One;
    
    /** @brief - Two key */
    static APPLICATION_API const FKey Two;
    
    /** @brief - Three key */
    static APPLICATION_API const FKey Three;
    
    /** @brief - Four key */
    static APPLICATION_API const FKey Four;
    
    /** @brief - Five key */
    static APPLICATION_API const FKey Five;
    
    /** @brief - Six key */
    static APPLICATION_API const FKey Six;
    
    /** @brief - Seven key */
    static APPLICATION_API const FKey Seven;
    
    /** @brief - Eight key */
    static APPLICATION_API const FKey Eight;
    
    /** @brief - Nine key */
    static APPLICATION_API const FKey Nine;

    // Letters

    /** @brief - A key */
    static APPLICATION_API const FKey A;
    
    /** @brief - B key */
    static APPLICATION_API const FKey B;
    
    /** @brief - C key */
    static APPLICATION_API const FKey C;
    
    /** @brief - D key */
    static APPLICATION_API const FKey D;
    
    /** @brief - E key */
    static APPLICATION_API const FKey E;
    
    /** @brief - F key */
    static APPLICATION_API const FKey F;
    
    /** @brief - G key */
    static APPLICATION_API const FKey G;
    
    /** @brief - H key */
    static APPLICATION_API const FKey H;
    
    /** @brief - I key */
    static APPLICATION_API const FKey I;
    
    /** @brief - J key */
    static APPLICATION_API const FKey J;
    
    /** @brief - K key */
    static APPLICATION_API const FKey K;
    
    /** @brief - L key */
    static APPLICATION_API const FKey L;
    
    /** @brief - M key */
    static APPLICATION_API const FKey M;
    
    /** @brief - N key */
    static APPLICATION_API const FKey N;
    
    /** @brief - O key */
    static APPLICATION_API const FKey O;
    
    /** @brief - P key */
    static APPLICATION_API const FKey P;
    
    /** @brief - Q key */
    static APPLICATION_API const FKey Q;
    
    /** @brief - R key */
    static APPLICATION_API const FKey R;
    
    /** @brief - S key */
    static APPLICATION_API const FKey S;
    
    /** @brief - T key */
    static APPLICATION_API const FKey T;
    
    /** @brief - U key */
    static APPLICATION_API const FKey U;
    
    /** @brief - V key */
    static APPLICATION_API const FKey V;
    
    /** @brief - W key */
    static APPLICATION_API const FKey W;
    
    /** @brief - X key */
    static APPLICATION_API const FKey X;
    
    /** @brief - Y key */
    static APPLICATION_API const FKey Y;
    
    /** @brief - Z key */
    static APPLICATION_API const FKey Z;

    // Function keys
    
    /** @brief - F1 key */
    static APPLICATION_API const FKey F1;
    
    /** @brief - F2 key */
    static APPLICATION_API const FKey F2;
    
    /** @brief - F3 key */
    static APPLICATION_API const FKey F3;
    
    /** @brief - F4 key */
    static APPLICATION_API const FKey F4;
    
    /** @brief - F5 key */
    static APPLICATION_API const FKey F5;
    
    /** @brief - F6 key */
    static APPLICATION_API const FKey F6;
    
    /** @brief - F7 key */
    static APPLICATION_API const FKey F7;
    
    /** @brief - F8 key */
    static APPLICATION_API const FKey F8;
    
    /** @brief - F9 key */
    static APPLICATION_API const FKey F9;
    
    /** @brief - F10 key */
    static APPLICATION_API const FKey F10;
    
    /** @brief - F11 key */
    static APPLICATION_API const FKey F11;
    
    /** @brief - F12 key */
    static APPLICATION_API const FKey F12;
    
    /** @brief - F13 key */
    static APPLICATION_API const FKey F13;
    
    /** @brief - F14 key */
    static APPLICATION_API const FKey F14;
    
    /** @brief - F15 key */
    static APPLICATION_API const FKey F15;
    
    /** @brief - F16 key */
    static APPLICATION_API const FKey F16;
    
    /** @brief - F17 key */
    static APPLICATION_API const FKey F17;
    
    /** @brief - F18 key */
    static APPLICATION_API const FKey F18;
    
    /** @brief - F19 key */
    static APPLICATION_API const FKey F19;
    
    /** @brief - F20 key */
    static APPLICATION_API const FKey F20;
    
    /** @brief - F21 key */
    static APPLICATION_API const FKey F21;
    
    /** @brief - F22 key */
    static APPLICATION_API const FKey F22;
    
    /** @brief - F23 key */
    static APPLICATION_API const FKey F23;
    
    /** @brief - F24 key */
    static APPLICATION_API const FKey F24;
    
    /** @brief - F25 key */
    static APPLICATION_API const FKey F25;

    // Keypad
    
    /** @brief - Keypad Zero key */
    static APPLICATION_API const FKey KeypadZero;
    
    /** @brief - Keypad One key */
    static APPLICATION_API const FKey KeypadOne;
    
    /** @brief - Keypad Two key */
    static APPLICATION_API const FKey KeypadTwo;
    
    /** @brief - Keypad Three key */
    static APPLICATION_API const FKey KeypadThree;
    
    /** @brief - Keypad Four key */
    static APPLICATION_API const FKey KeypadFour;
    
    /** @brief - Keypad Five key */
    static APPLICATION_API const FKey KeypadFive;
    
    /** @brief - Keypad Six key */
    static APPLICATION_API const FKey KeypadSix;
    
    /** @brief - Keypad Seven key */
    static APPLICATION_API const FKey KeypadSeven;
    
    /** @brief - Keypad Eight key */
    static APPLICATION_API const FKey KeypadEight;
    
    /** @brief - Keypad Nine key */
    static APPLICATION_API const FKey KeypadNine;
    
    /** @brief - Keypad Decimal key */
    static APPLICATION_API const FKey KeypadDecimal;
    
    /** @brief - Keypad Divide key */
    static APPLICATION_API const FKey KeypadDivide;
    
    /** @brief - Keypad Multiply key */
    static APPLICATION_API const FKey KeypadMultiply;
    
    /** @brief - Keypad Subtract key */
    static APPLICATION_API const FKey KeypadSubtract;
    
    /** @brief - Keypad Add key */
    static APPLICATION_API const FKey KeypadAdd;
    
    /** @brief - Keypad Enter key */
    static APPLICATION_API const FKey KeypadEnter;
    
    /** @brief - Keypad Equal key */
    static APPLICATION_API const FKey KeypadEqual;

    // Ctrl, Shift, Alt, Etc.
    
    /** @brief - Left Shift key */
    static APPLICATION_API const FKey LeftShift;
    
    /** @brief - Left Control key */
    static APPLICATION_API const FKey LeftControl;
    
    /** @brief - Left Alt key */
    static APPLICATION_API const FKey LeftAlt;
    
    /** @brief - Left Super key */
    static APPLICATION_API const FKey LeftSuper;
    
    /** @brief - Right Shift key */
    static APPLICATION_API const FKey RightShift;
    
    /** @brief - Right Control key */
    static APPLICATION_API const FKey RightControl;
    
    /** @brief - Right Alt key */
    static APPLICATION_API const FKey RightAlt;
    
    /** @brief - Right Super key */
    static APPLICATION_API const FKey RightSuper;
    
    /** @brief - Menu key */
    static APPLICATION_API const FKey Menu;

    // Other
    
    /** @brief - Space key */
    static APPLICATION_API const FKey Space;
    
    /** @brief - Apostrophe key */
    static APPLICATION_API const FKey Apostrophe;
    
    /** @brief - Comma key */
    static APPLICATION_API const FKey Comma;
    
    /** @brief - Minus key */
    static APPLICATION_API const FKey Minus;
    
    /** @brief - Period key */
    static APPLICATION_API const FKey Period;
    
    /** @brief - Slash key */
    static APPLICATION_API const FKey Slash;
    
    /** @brief - Semicolon key */
    static APPLICATION_API const FKey Semicolon;
    
    /** @brief - Equal key */
    static APPLICATION_API const FKey Equal;
    
    /** @brief - Left Bracket key */
    static APPLICATION_API const FKey LeftBracket;
    
    /** @brief - Backslash key */
    static APPLICATION_API const FKey Backslash;
    
    /** @brief - Right Bracket key */
    static APPLICATION_API const FKey RightBracket;
    
    /** @brief - Grave Accent key */
    static APPLICATION_API const FKey GraveAccent;
    
    /** @brief - World1 key */
    static APPLICATION_API const FKey World1;
    
    /** @brief - World2 key */
    static APPLICATION_API const FKey World2;
    
    /** @brief - Escape key */
    static APPLICATION_API const FKey Escape;
    
    /** @brief - Enter key */
    static APPLICATION_API const FKey Enter;
    
    /** @brief - Tab key */
    static APPLICATION_API const FKey Tab;
    
    /** @brief - Backspace key */
    static APPLICATION_API const FKey Backspace;
    
    /** @brief - Insert key */
    static APPLICATION_API const FKey Insert;
    
    /** @brief - Delete key */
    static APPLICATION_API const FKey Delete;
    
    /** @brief - Right key */
    static APPLICATION_API const FKey Right;
    
    /** @brief - Left key */
    static APPLICATION_API const FKey Left;
    
    /** @brief - Down key */
    static APPLICATION_API const FKey Down;
    
    /** @brief - Up key */
    static APPLICATION_API const FKey Up;
    
    /** @brief - Page Up key */
    static APPLICATION_API const FKey PageUp;
    
    /** @brief - Page Down key */
    static APPLICATION_API const FKey PageDown;
    
    /** @brief - Home key */
    static APPLICATION_API const FKey Home;
    
    /** @brief - End key */
    static APPLICATION_API const FKey End;
    
    /** @brief - Caps Lock key */
    static APPLICATION_API const FKey CapsLock;
    
    /** @brief - Scroll Lock key */
    static APPLICATION_API const FKey ScrollLock;
    
    /** @brief - Num Lock key */
    static APPLICATION_API const FKey NumLock;
    
    /** @brief - Print Screen key */
    static APPLICATION_API const FKey PrintScreen;
    
    /** @brief - Pause key */
    static APPLICATION_API const FKey Pause;

    // Mouse buttons

    /** @brief - Mouse left button */
    static APPLICATION_API const FKey MouseButtonLeft;
    
    /** @brief - Mouse right button */
    static APPLICATION_API const FKey MouseButtonRight;
    
    /** @brief - Mouse middle button */
    static APPLICATION_API const FKey MouseButtonMiddle;
    
    /** @brief - Mouse thumb1 button */
    static APPLICATION_API const FKey MouseButtonThumb1;
    
    /** @brief - Mouse thumb2 button */
    static APPLICATION_API const FKey MouseButtonThumb2;

    // Gamepad buttons
    
    /** @brief - Gamepad DPad Up button */
    static APPLICATION_API const FKey GamepadDPadUp;
    
    /** @brief - Gamepad DPad Down button */
    static APPLICATION_API const FKey GamepadDPadDown;
    
    /** @brief - Gamepad DPad Left button */
    static APPLICATION_API const FKey GamepadDPadLeft;
    
    /** @brief - Gamepad DPad Right button */
    static APPLICATION_API const FKey GamepadDPadRight;
    
    /** @brief - Gamepad Face Up button */
    static APPLICATION_API const FKey GamepadFaceUp;
    
    /** @brief - Gamepad Face Down button */
    static APPLICATION_API const FKey GamepadFaceDown;
    
    /** @brief - Gamepad Face Left button */
    static APPLICATION_API const FKey GamepadFaceLeft;
    
    /** @brief - Gamepad Face Right button */
    static APPLICATION_API const FKey GamepadFaceRight;
    
    /** @brief - Gamepad Right Trigger button */
    static APPLICATION_API const FKey GamepadRightTrigger;
    
    /** @brief - Gamepad Left Trigger button */
    static APPLICATION_API const FKey GamepadLeftTrigger;
    
    /** @brief - Gamepad Right Shoulder button */
    static APPLICATION_API const FKey GamepadRightShoulder;
    
    /** @brief - Gamepad Left Shoulder button */
    static APPLICATION_API const FKey GamepadLeftShoulder;
    
    /** @brief - Gamepad Start button */
    static APPLICATION_API const FKey GamepadStart;
    
    /** @brief - Gamepad Back button */
    static APPLICATION_API const FKey GamepadBack;
};