#if PLATFORM_MACOS
#include "MacInputMapper.h"

TStaticArray<EKeyName::Type, FMacInputMapper::NumKeys>         FMacInputMapper::KeyCodeFromScanCodeTable;
TStaticArray<uint16, FMacInputMapper::NumKeys>                 FMacInputMapper::ScanCodeFromKeyCodeTable;
TStaticArray<EMouseButtonName::Type, EMouseButtonName::Count> FMacInputMapper::ButtonFromButtonIndex;
TStaticArray<uint8, EMouseButtonName::Count>                  FMacInputMapper::ButtonIndexFromButton;

void FMacInputMapper::Initialize()
{
    KeyCodeFromScanCodeTable.Memzero();
    ScanCodeFromKeyCodeTable.Memzero();

    KeyCodeFromScanCodeTable[0x33] = EKeyName::Backspace;
    KeyCodeFromScanCodeTable[0x30] = EKeyName::Tab;
    KeyCodeFromScanCodeTable[0x24] = EKeyName::Enter;
    KeyCodeFromScanCodeTable[0x39] = EKeyName::CapsLock;
    KeyCodeFromScanCodeTable[0x31] = EKeyName::Space;
    KeyCodeFromScanCodeTable[0x74] = EKeyName::PageUp;
    KeyCodeFromScanCodeTable[0x79] = EKeyName::PageDown;
    KeyCodeFromScanCodeTable[0x77] = EKeyName::End;
    KeyCodeFromScanCodeTable[0x73] = EKeyName::Home;
    KeyCodeFromScanCodeTable[0x7B] = EKeyName::Left;
    KeyCodeFromScanCodeTable[0x7E] = EKeyName::Up;
    KeyCodeFromScanCodeTable[0x7C] = EKeyName::Right;
    KeyCodeFromScanCodeTable[0x7D] = EKeyName::Down;
    KeyCodeFromScanCodeTable[0x72] = EKeyName::Insert;
    KeyCodeFromScanCodeTable[0x75] = EKeyName::Delete;
    KeyCodeFromScanCodeTable[0x35] = EKeyName::Escape;
    KeyCodeFromScanCodeTable[0x1D] = EKeyName::Zero;
    KeyCodeFromScanCodeTable[0x12] = EKeyName::One;
    KeyCodeFromScanCodeTable[0x13] = EKeyName::Two;
    KeyCodeFromScanCodeTable[0x14] = EKeyName::Three;
    KeyCodeFromScanCodeTable[0x15] = EKeyName::Four;
    KeyCodeFromScanCodeTable[0x17] = EKeyName::Five;
    KeyCodeFromScanCodeTable[0x16] = EKeyName::Six;
    KeyCodeFromScanCodeTable[0x1A] = EKeyName::Seven;
    KeyCodeFromScanCodeTable[0x1C] = EKeyName::Eight;
    KeyCodeFromScanCodeTable[0x19] = EKeyName::Nine;
    KeyCodeFromScanCodeTable[0x00] = EKeyName::A;
    KeyCodeFromScanCodeTable[0x0B] = EKeyName::B;
    KeyCodeFromScanCodeTable[0x08] = EKeyName::C;
    KeyCodeFromScanCodeTable[0x02] = EKeyName::D;
    KeyCodeFromScanCodeTable[0x0E] = EKeyName::E;
    KeyCodeFromScanCodeTable[0x03] = EKeyName::F;
    KeyCodeFromScanCodeTable[0x05] = EKeyName::G;
    KeyCodeFromScanCodeTable[0x04] = EKeyName::H;
    KeyCodeFromScanCodeTable[0x22] = EKeyName::I;
    KeyCodeFromScanCodeTable[0x26] = EKeyName::J;
    KeyCodeFromScanCodeTable[0x28] = EKeyName::K;
    KeyCodeFromScanCodeTable[0x25] = EKeyName::L;
    KeyCodeFromScanCodeTable[0x2E] = EKeyName::M;
    KeyCodeFromScanCodeTable[0x2D] = EKeyName::N;
    KeyCodeFromScanCodeTable[0x1F] = EKeyName::O;
    KeyCodeFromScanCodeTable[0x23] = EKeyName::P;
    KeyCodeFromScanCodeTable[0x0C] = EKeyName::Q;
    KeyCodeFromScanCodeTable[0x0F] = EKeyName::R;
    KeyCodeFromScanCodeTable[0x01] = EKeyName::S;
    KeyCodeFromScanCodeTable[0x11] = EKeyName::T;
    KeyCodeFromScanCodeTable[0x20] = EKeyName::U;
    KeyCodeFromScanCodeTable[0x09] = EKeyName::V;
    KeyCodeFromScanCodeTable[0x0D] = EKeyName::W;
    KeyCodeFromScanCodeTable[0x07] = EKeyName::X;
    KeyCodeFromScanCodeTable[0x10] = EKeyName::Y;
    KeyCodeFromScanCodeTable[0x06] = EKeyName::Z;
    KeyCodeFromScanCodeTable[0x52] = EKeyName::KeypadZero;
    KeyCodeFromScanCodeTable[0x53] = EKeyName::KeypadOne;
    KeyCodeFromScanCodeTable[0x54] = EKeyName::KeypadTwo;
    KeyCodeFromScanCodeTable[0x55] = EKeyName::KeypadThree;
    KeyCodeFromScanCodeTable[0x56] = EKeyName::KeypadFour;
    KeyCodeFromScanCodeTable[0x57] = EKeyName::KeypadFive;
    KeyCodeFromScanCodeTable[0x58] = EKeyName::KeypadSix;
    KeyCodeFromScanCodeTable[0x59] = EKeyName::KeypadSeven;
    KeyCodeFromScanCodeTable[0x5B] = EKeyName::KeypadEight;
    KeyCodeFromScanCodeTable[0x5C] = EKeyName::KeypadNine;
    KeyCodeFromScanCodeTable[0x45] = EKeyName::KeypadAdd;
    KeyCodeFromScanCodeTable[0x41] = EKeyName::KeypadDecimal;
    KeyCodeFromScanCodeTable[0x4B] = EKeyName::KeypadDivide;
    KeyCodeFromScanCodeTable[0x4C] = EKeyName::KeypadEnter;
    KeyCodeFromScanCodeTable[0x51] = EKeyName::KeypadEqual;
    KeyCodeFromScanCodeTable[0x43] = EKeyName::KeypadMultiply;
    KeyCodeFromScanCodeTable[0x4E] = EKeyName::KeypadSubtract;
    KeyCodeFromScanCodeTable[0x7A] = EKeyName::F1;
    KeyCodeFromScanCodeTable[0x78] = EKeyName::F2;
    KeyCodeFromScanCodeTable[0x63] = EKeyName::F3;
    KeyCodeFromScanCodeTable[0x76] = EKeyName::F4;
    KeyCodeFromScanCodeTable[0x60] = EKeyName::F5;
    KeyCodeFromScanCodeTable[0x61] = EKeyName::F6;
    KeyCodeFromScanCodeTable[0x62] = EKeyName::F7;
    KeyCodeFromScanCodeTable[0x64] = EKeyName::F8;
    KeyCodeFromScanCodeTable[0x65] = EKeyName::F9;
    KeyCodeFromScanCodeTable[0x6D] = EKeyName::F10;
    KeyCodeFromScanCodeTable[0x67] = EKeyName::F11;
    KeyCodeFromScanCodeTable[0x6F] = EKeyName::F12;
    KeyCodeFromScanCodeTable[0x69] = EKeyName::F13;
    KeyCodeFromScanCodeTable[0x6B] = EKeyName::F14;
    KeyCodeFromScanCodeTable[0x71] = EKeyName::F15;
    KeyCodeFromScanCodeTable[0x6A] = EKeyName::F16;
    KeyCodeFromScanCodeTable[0x40] = EKeyName::F17;
    KeyCodeFromScanCodeTable[0x4F] = EKeyName::F18;
    KeyCodeFromScanCodeTable[0x50] = EKeyName::F19;
    KeyCodeFromScanCodeTable[0x5A] = EKeyName::F20;
    KeyCodeFromScanCodeTable[0x47] = EKeyName::NumLock;
    KeyCodeFromScanCodeTable[0x29] = EKeyName::Semicolon;
    KeyCodeFromScanCodeTable[0x2B] = EKeyName::Comma;
    KeyCodeFromScanCodeTable[0x1B] = EKeyName::Minus;
    KeyCodeFromScanCodeTable[0x2F] = EKeyName::Period;
    KeyCodeFromScanCodeTable[0x32] = EKeyName::GraveAccent;
    KeyCodeFromScanCodeTable[0x21] = EKeyName::LeftBracket;
    KeyCodeFromScanCodeTable[0x1E] = EKeyName::RightBracket;
    KeyCodeFromScanCodeTable[0x27] = EKeyName::Apostrophe;
    KeyCodeFromScanCodeTable[0x2A] = EKeyName::Backslash;
    KeyCodeFromScanCodeTable[0x38] = EKeyName::LeftShift;
    KeyCodeFromScanCodeTable[0x3B] = EKeyName::LeftControl;
    KeyCodeFromScanCodeTable[0x3A] = EKeyName::LeftAlt;
    KeyCodeFromScanCodeTable[0x37] = EKeyName::LeftSuper;
    KeyCodeFromScanCodeTable[0x3C] = EKeyName::RightShift;
    KeyCodeFromScanCodeTable[0x3E] = EKeyName::LeftControl;
    KeyCodeFromScanCodeTable[0x36] = EKeyName::RightSuper;
    KeyCodeFromScanCodeTable[0x3D] = EKeyName::LeftAlt;
    KeyCodeFromScanCodeTable[0x6E] = EKeyName::Menu;
    KeyCodeFromScanCodeTable[0x18] = EKeyName::Equal;
    KeyCodeFromScanCodeTable[0x2C] = EKeyName::Slash;
    KeyCodeFromScanCodeTable[0x0A] = EKeyName::World1;

    for (uint16 Index = 0; Index < NumKeys; ++Index)
    {
        if (KeyCodeFromScanCodeTable[Index] != EKeyName::Unknown)
        {
            ScanCodeFromKeyCodeTable[KeyCodeFromScanCodeTable[Index]] = Index;
        }
    }

    ButtonFromButtonIndex.Memzero();
    ButtonIndexFromButton.Memzero();

    ButtonFromButtonIndex[0] = EMouseButtonName::Left;
    ButtonFromButtonIndex[1] = EMouseButtonName::Right;
    ButtonFromButtonIndex[2] = EMouseButtonName::Middle;

    for (uint8 Index = 0; Index < EMouseButtonName::Count; ++Index)
    {
        if (ButtonFromButtonIndex[Index] != EMouseButtonName::Unknown)
        {
            ButtonIndexFromButton[ButtonFromButtonIndex[Index]] = Index;
        }
    }
}

#endif

