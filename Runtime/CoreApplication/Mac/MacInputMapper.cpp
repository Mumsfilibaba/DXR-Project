#if PLATFORM_MACOS
#include "CoreApplication/Mac/MacInputMapper.h"

TStaticArray<EKeyboardKeyName::Type, FMacInputMapper::NumKeys> FMacInputMapper::KeyCodeFromScanCodeTable;
TStaticArray<uint16, FMacInputMapper::NumKeys>                 FMacInputMapper::ScanCodeFromKeyCodeTable;
TStaticArray<EMouseButtonName::Type, EMouseButtonName::Count>  FMacInputMapper::ButtonFromButtonIndex;
TStaticArray<uint8, EMouseButtonName::Count>                   FMacInputMapper::ButtonIndexFromButton;

void FMacInputMapper::Initialize()
{
    KeyCodeFromScanCodeTable.Memzero();
    ScanCodeFromKeyCodeTable.Memzero();

    KeyCodeFromScanCodeTable[0x33] = EKeyboardKeyName::Backspace;
    KeyCodeFromScanCodeTable[0x30] = EKeyboardKeyName::Tab;
    KeyCodeFromScanCodeTable[0x24] = EKeyboardKeyName::Enter;
    KeyCodeFromScanCodeTable[0x39] = EKeyboardKeyName::CapsLock;
    KeyCodeFromScanCodeTable[0x31] = EKeyboardKeyName::Space;
    KeyCodeFromScanCodeTable[0x74] = EKeyboardKeyName::PageUp;
    KeyCodeFromScanCodeTable[0x79] = EKeyboardKeyName::PageDown;
    KeyCodeFromScanCodeTable[0x77] = EKeyboardKeyName::End;
    KeyCodeFromScanCodeTable[0x73] = EKeyboardKeyName::Home;
    KeyCodeFromScanCodeTable[0x7B] = EKeyboardKeyName::Left;
    KeyCodeFromScanCodeTable[0x7E] = EKeyboardKeyName::Up;
    KeyCodeFromScanCodeTable[0x7C] = EKeyboardKeyName::Right;
    KeyCodeFromScanCodeTable[0x7D] = EKeyboardKeyName::Down;
    KeyCodeFromScanCodeTable[0x72] = EKeyboardKeyName::Insert;
    KeyCodeFromScanCodeTable[0x75] = EKeyboardKeyName::Delete;
    KeyCodeFromScanCodeTable[0x35] = EKeyboardKeyName::Escape;
    KeyCodeFromScanCodeTable[0x1D] = EKeyboardKeyName::Zero;
    KeyCodeFromScanCodeTable[0x12] = EKeyboardKeyName::One;
    KeyCodeFromScanCodeTable[0x13] = EKeyboardKeyName::Two;
    KeyCodeFromScanCodeTable[0x14] = EKeyboardKeyName::Three;
    KeyCodeFromScanCodeTable[0x15] = EKeyboardKeyName::Four;
    KeyCodeFromScanCodeTable[0x17] = EKeyboardKeyName::Five;
    KeyCodeFromScanCodeTable[0x16] = EKeyboardKeyName::Six;
    KeyCodeFromScanCodeTable[0x1A] = EKeyboardKeyName::Seven;
    KeyCodeFromScanCodeTable[0x1C] = EKeyboardKeyName::Eight;
    KeyCodeFromScanCodeTable[0x19] = EKeyboardKeyName::Nine;
    KeyCodeFromScanCodeTable[0x00] = EKeyboardKeyName::A;
    KeyCodeFromScanCodeTable[0x0B] = EKeyboardKeyName::B;
    KeyCodeFromScanCodeTable[0x08] = EKeyboardKeyName::C;
    KeyCodeFromScanCodeTable[0x02] = EKeyboardKeyName::D;
    KeyCodeFromScanCodeTable[0x0E] = EKeyboardKeyName::E;
    KeyCodeFromScanCodeTable[0x03] = EKeyboardKeyName::F;
    KeyCodeFromScanCodeTable[0x05] = EKeyboardKeyName::G;
    KeyCodeFromScanCodeTable[0x04] = EKeyboardKeyName::H;
    KeyCodeFromScanCodeTable[0x22] = EKeyboardKeyName::I;
    KeyCodeFromScanCodeTable[0x26] = EKeyboardKeyName::J;
    KeyCodeFromScanCodeTable[0x28] = EKeyboardKeyName::K;
    KeyCodeFromScanCodeTable[0x25] = EKeyboardKeyName::L;
    KeyCodeFromScanCodeTable[0x2E] = EKeyboardKeyName::M;
    KeyCodeFromScanCodeTable[0x2D] = EKeyboardKeyName::N;
    KeyCodeFromScanCodeTable[0x1F] = EKeyboardKeyName::O;
    KeyCodeFromScanCodeTable[0x23] = EKeyboardKeyName::P;
    KeyCodeFromScanCodeTable[0x0C] = EKeyboardKeyName::Q;
    KeyCodeFromScanCodeTable[0x0F] = EKeyboardKeyName::R;
    KeyCodeFromScanCodeTable[0x01] = EKeyboardKeyName::S;
    KeyCodeFromScanCodeTable[0x11] = EKeyboardKeyName::T;
    KeyCodeFromScanCodeTable[0x20] = EKeyboardKeyName::U;
    KeyCodeFromScanCodeTable[0x09] = EKeyboardKeyName::V;
    KeyCodeFromScanCodeTable[0x0D] = EKeyboardKeyName::W;
    KeyCodeFromScanCodeTable[0x07] = EKeyboardKeyName::X;
    KeyCodeFromScanCodeTable[0x10] = EKeyboardKeyName::Y;
    KeyCodeFromScanCodeTable[0x06] = EKeyboardKeyName::Z;
    KeyCodeFromScanCodeTable[0x52] = EKeyboardKeyName::KeypadZero;
    KeyCodeFromScanCodeTable[0x53] = EKeyboardKeyName::KeypadOne;
    KeyCodeFromScanCodeTable[0x54] = EKeyboardKeyName::KeypadTwo;
    KeyCodeFromScanCodeTable[0x55] = EKeyboardKeyName::KeypadThree;
    KeyCodeFromScanCodeTable[0x56] = EKeyboardKeyName::KeypadFour;
    KeyCodeFromScanCodeTable[0x57] = EKeyboardKeyName::KeypadFive;
    KeyCodeFromScanCodeTable[0x58] = EKeyboardKeyName::KeypadSix;
    KeyCodeFromScanCodeTable[0x59] = EKeyboardKeyName::KeypadSeven;
    KeyCodeFromScanCodeTable[0x5B] = EKeyboardKeyName::KeypadEight;
    KeyCodeFromScanCodeTable[0x5C] = EKeyboardKeyName::KeypadNine;
    KeyCodeFromScanCodeTable[0x45] = EKeyboardKeyName::KeypadAdd;
    KeyCodeFromScanCodeTable[0x41] = EKeyboardKeyName::KeypadDecimal;
    KeyCodeFromScanCodeTable[0x4B] = EKeyboardKeyName::KeypadDivide;
    KeyCodeFromScanCodeTable[0x4C] = EKeyboardKeyName::KeypadEnter;
    KeyCodeFromScanCodeTable[0x51] = EKeyboardKeyName::KeypadEqual;
    KeyCodeFromScanCodeTable[0x43] = EKeyboardKeyName::KeypadMultiply;
    KeyCodeFromScanCodeTable[0x4E] = EKeyboardKeyName::KeypadSubtract;
    KeyCodeFromScanCodeTable[0x7A] = EKeyboardKeyName::F1;
    KeyCodeFromScanCodeTable[0x78] = EKeyboardKeyName::F2;
    KeyCodeFromScanCodeTable[0x63] = EKeyboardKeyName::F3;
    KeyCodeFromScanCodeTable[0x76] = EKeyboardKeyName::F4;
    KeyCodeFromScanCodeTable[0x60] = EKeyboardKeyName::F5;
    KeyCodeFromScanCodeTable[0x61] = EKeyboardKeyName::F6;
    KeyCodeFromScanCodeTable[0x62] = EKeyboardKeyName::F7;
    KeyCodeFromScanCodeTable[0x64] = EKeyboardKeyName::F8;
    KeyCodeFromScanCodeTable[0x65] = EKeyboardKeyName::F9;
    KeyCodeFromScanCodeTable[0x6D] = EKeyboardKeyName::F10;
    KeyCodeFromScanCodeTable[0x67] = EKeyboardKeyName::F11;
    KeyCodeFromScanCodeTable[0x6F] = EKeyboardKeyName::F12;
    KeyCodeFromScanCodeTable[0x69] = EKeyboardKeyName::F13;
    KeyCodeFromScanCodeTable[0x6B] = EKeyboardKeyName::F14;
    KeyCodeFromScanCodeTable[0x71] = EKeyboardKeyName::F15;
    KeyCodeFromScanCodeTable[0x6A] = EKeyboardKeyName::F16;
    KeyCodeFromScanCodeTable[0x40] = EKeyboardKeyName::F17;
    KeyCodeFromScanCodeTable[0x4F] = EKeyboardKeyName::F18;
    KeyCodeFromScanCodeTable[0x50] = EKeyboardKeyName::F19;
    KeyCodeFromScanCodeTable[0x5A] = EKeyboardKeyName::F20;
    KeyCodeFromScanCodeTable[0x47] = EKeyboardKeyName::NumLock;
    KeyCodeFromScanCodeTable[0x29] = EKeyboardKeyName::Semicolon;
    KeyCodeFromScanCodeTable[0x2B] = EKeyboardKeyName::Comma;
    KeyCodeFromScanCodeTable[0x1B] = EKeyboardKeyName::Minus;
    KeyCodeFromScanCodeTable[0x2F] = EKeyboardKeyName::Period;
    KeyCodeFromScanCodeTable[0x32] = EKeyboardKeyName::GraveAccent;
    KeyCodeFromScanCodeTable[0x21] = EKeyboardKeyName::LeftBracket;
    KeyCodeFromScanCodeTable[0x1E] = EKeyboardKeyName::RightBracket;
    KeyCodeFromScanCodeTable[0x27] = EKeyboardKeyName::Apostrophe;
    KeyCodeFromScanCodeTable[0x2A] = EKeyboardKeyName::Backslash;
    KeyCodeFromScanCodeTable[0x38] = EKeyboardKeyName::LeftShift;
    KeyCodeFromScanCodeTable[0x3B] = EKeyboardKeyName::LeftControl;
    KeyCodeFromScanCodeTable[0x3A] = EKeyboardKeyName::LeftAlt;
    KeyCodeFromScanCodeTable[0x37] = EKeyboardKeyName::LeftSuper;
    KeyCodeFromScanCodeTable[0x3C] = EKeyboardKeyName::RightShift;
    KeyCodeFromScanCodeTable[0x3E] = EKeyboardKeyName::LeftControl;
    KeyCodeFromScanCodeTable[0x36] = EKeyboardKeyName::RightSuper;
    KeyCodeFromScanCodeTable[0x3D] = EKeyboardKeyName::LeftAlt;
    KeyCodeFromScanCodeTable[0x6E] = EKeyboardKeyName::Menu;
    KeyCodeFromScanCodeTable[0x18] = EKeyboardKeyName::Equal;
    KeyCodeFromScanCodeTable[0x2C] = EKeyboardKeyName::Slash;
    KeyCodeFromScanCodeTable[0x0A] = EKeyboardKeyName::World1;

    for (uint16 Index = 0; Index < NumKeys; ++Index)
    {
        if (KeyCodeFromScanCodeTable[Index] != EKeyboardKeyName::Unknown)
        {
            ScanCodeFromKeyCodeTable[KeyCodeFromScanCodeTable[Index]] = Index;
        }
    }

    ButtonFromButtonIndex.Memzero();
    ButtonIndexFromButton.Memzero();

    ButtonFromButtonIndex[0] = EMouseButtonName::Left;
    ButtonFromButtonIndex[1] = EMouseButtonName::Right;
    ButtonFromButtonIndex[2] = EMouseButtonName::Middle;
    ButtonFromButtonIndex[3] = EMouseButtonName::Thumb1;
    ButtonFromButtonIndex[4] = EMouseButtonName::Thumb2;

    for (uint8 Index = 0; Index < EMouseButtonName::Count; ++Index)
    {
        if (ButtonFromButtonIndex[Index] != EMouseButtonName::Unknown)
        {
            ButtonIndexFromButton[ButtonFromButtonIndex[Index]] = Index;
        }
    }
}

#endif

