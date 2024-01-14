#include "WindowsInputMapper.h"

TStaticArray<uint16, FWindowsInputMapper::NumKeys> FWindowsInputMapper::ScanCodeFromKeyCodeTable;
TStaticArray<EKeyboardKeyName::Type  , FWindowsInputMapper::NumKeys> FWindowsInputMapper::KeyCodeFromScanCodeTable;

void FWindowsInputMapper::Initialize()
{
    KeyCodeFromScanCodeTable.Memzero(); 
    ScanCodeFromKeyCodeTable.Memzero();

    KeyCodeFromScanCodeTable[0x00B] = EKeyboardKeyName::Zero;
    KeyCodeFromScanCodeTable[0x002] = EKeyboardKeyName::One;
    KeyCodeFromScanCodeTable[0x003] = EKeyboardKeyName::Two;
    KeyCodeFromScanCodeTable[0x004] = EKeyboardKeyName::Three;
    KeyCodeFromScanCodeTable[0x005] = EKeyboardKeyName::Four;
    KeyCodeFromScanCodeTable[0x006] = EKeyboardKeyName::Five;
    KeyCodeFromScanCodeTable[0x007] = EKeyboardKeyName::Six;
    KeyCodeFromScanCodeTable[0x008] = EKeyboardKeyName::Seven;
    KeyCodeFromScanCodeTable[0x009] = EKeyboardKeyName::Eight;
    KeyCodeFromScanCodeTable[0x00A] = EKeyboardKeyName::Nine;

    KeyCodeFromScanCodeTable[0x01E] = EKeyboardKeyName::A;
    KeyCodeFromScanCodeTable[0x030] = EKeyboardKeyName::B;
    KeyCodeFromScanCodeTable[0x02E] = EKeyboardKeyName::C;
    KeyCodeFromScanCodeTable[0x020] = EKeyboardKeyName::D;
    KeyCodeFromScanCodeTable[0x012] = EKeyboardKeyName::E;
    KeyCodeFromScanCodeTable[0x021] = EKeyboardKeyName::F;
    KeyCodeFromScanCodeTable[0x022] = EKeyboardKeyName::G;
    KeyCodeFromScanCodeTable[0x023] = EKeyboardKeyName::H;
    KeyCodeFromScanCodeTable[0x017] = EKeyboardKeyName::I;
    KeyCodeFromScanCodeTable[0x024] = EKeyboardKeyName::J;
    KeyCodeFromScanCodeTable[0x025] = EKeyboardKeyName::K;
    KeyCodeFromScanCodeTable[0x026] = EKeyboardKeyName::L;
    KeyCodeFromScanCodeTable[0x032] = EKeyboardKeyName::M;
    KeyCodeFromScanCodeTable[0x031] = EKeyboardKeyName::N;
    KeyCodeFromScanCodeTable[0x018] = EKeyboardKeyName::O;
    KeyCodeFromScanCodeTable[0x019] = EKeyboardKeyName::P;
    KeyCodeFromScanCodeTable[0x010] = EKeyboardKeyName::Q;
    KeyCodeFromScanCodeTable[0x013] = EKeyboardKeyName::R;
    KeyCodeFromScanCodeTable[0x01F] = EKeyboardKeyName::S;
    KeyCodeFromScanCodeTable[0x014] = EKeyboardKeyName::T;
    KeyCodeFromScanCodeTable[0x016] = EKeyboardKeyName::U;
    KeyCodeFromScanCodeTable[0x02F] = EKeyboardKeyName::V;
    KeyCodeFromScanCodeTable[0x011] = EKeyboardKeyName::W;
    KeyCodeFromScanCodeTable[0x02D] = EKeyboardKeyName::X;
    KeyCodeFromScanCodeTable[0x015] = EKeyboardKeyName::Y;
    KeyCodeFromScanCodeTable[0x02C] = EKeyboardKeyName::Z;

    KeyCodeFromScanCodeTable[0x03B] = EKeyboardKeyName::F1;
    KeyCodeFromScanCodeTable[0x03C] = EKeyboardKeyName::F2;
    KeyCodeFromScanCodeTable[0x03D] = EKeyboardKeyName::F3;
    KeyCodeFromScanCodeTable[0x03E] = EKeyboardKeyName::F4;
    KeyCodeFromScanCodeTable[0x03F] = EKeyboardKeyName::F5;
    KeyCodeFromScanCodeTable[0x040] = EKeyboardKeyName::F6;
    KeyCodeFromScanCodeTable[0x041] = EKeyboardKeyName::F7;
    KeyCodeFromScanCodeTable[0x042] = EKeyboardKeyName::F8;
    KeyCodeFromScanCodeTable[0x043] = EKeyboardKeyName::F9;
    KeyCodeFromScanCodeTable[0x044] = EKeyboardKeyName::F10;
    KeyCodeFromScanCodeTable[0x057] = EKeyboardKeyName::F11;
    KeyCodeFromScanCodeTable[0x058] = EKeyboardKeyName::F12;
    KeyCodeFromScanCodeTable[0x064] = EKeyboardKeyName::F13;
    KeyCodeFromScanCodeTable[0x065] = EKeyboardKeyName::F14;
    KeyCodeFromScanCodeTable[0x066] = EKeyboardKeyName::F15;
    KeyCodeFromScanCodeTable[0x067] = EKeyboardKeyName::F16;
    KeyCodeFromScanCodeTable[0x068] = EKeyboardKeyName::F17;
    KeyCodeFromScanCodeTable[0x069] = EKeyboardKeyName::F18;
    KeyCodeFromScanCodeTable[0x06A] = EKeyboardKeyName::F19;
    KeyCodeFromScanCodeTable[0x06B] = EKeyboardKeyName::F20;
    KeyCodeFromScanCodeTable[0x06C] = EKeyboardKeyName::F21;
    KeyCodeFromScanCodeTable[0x06D] = EKeyboardKeyName::F22;
    KeyCodeFromScanCodeTable[0x06E] = EKeyboardKeyName::F23;
    KeyCodeFromScanCodeTable[0x076] = EKeyboardKeyName::F24;

    KeyCodeFromScanCodeTable[0x052] = EKeyboardKeyName::KeypadZero;
    KeyCodeFromScanCodeTable[0x04F] = EKeyboardKeyName::KeypadOne;
    KeyCodeFromScanCodeTable[0x050] = EKeyboardKeyName::KeypadTwo;
    KeyCodeFromScanCodeTable[0x051] = EKeyboardKeyName::KeypadThree;
    KeyCodeFromScanCodeTable[0x04B] = EKeyboardKeyName::KeypadFour;
    KeyCodeFromScanCodeTable[0x04C] = EKeyboardKeyName::KeypadFive;
    KeyCodeFromScanCodeTable[0x04D] = EKeyboardKeyName::KeypadSix;
    KeyCodeFromScanCodeTable[0x047] = EKeyboardKeyName::KeypadSeven;
    KeyCodeFromScanCodeTable[0x048] = EKeyboardKeyName::KeypadEight;
    KeyCodeFromScanCodeTable[0x049] = EKeyboardKeyName::KeypadNine;
    KeyCodeFromScanCodeTable[0x04E] = EKeyboardKeyName::KeypadAdd;
    KeyCodeFromScanCodeTable[0x053] = EKeyboardKeyName::KeypadDecimal;
    KeyCodeFromScanCodeTable[0x135] = EKeyboardKeyName::KeypadDivide;
    KeyCodeFromScanCodeTable[0x11C] = EKeyboardKeyName::KeypadEnter;
    KeyCodeFromScanCodeTable[0x059] = EKeyboardKeyName::KeypadEqual;
    KeyCodeFromScanCodeTable[0x037] = EKeyboardKeyName::KeypadMultiply;
    KeyCodeFromScanCodeTable[0x04A] = EKeyboardKeyName::KeypadSubtract;

    KeyCodeFromScanCodeTable[0x02A] = EKeyboardKeyName::LeftShift;
    KeyCodeFromScanCodeTable[0x036] = EKeyboardKeyName::RightShift;
    KeyCodeFromScanCodeTable[0x01D] = EKeyboardKeyName::LeftControl;
    KeyCodeFromScanCodeTable[0x11D] = EKeyboardKeyName::RightControl;
    KeyCodeFromScanCodeTable[0x038] = EKeyboardKeyName::LeftAlt;
    KeyCodeFromScanCodeTable[0x138] = EKeyboardKeyName::RightAlt;
    KeyCodeFromScanCodeTable[0x15B] = EKeyboardKeyName::LeftSuper;
    KeyCodeFromScanCodeTable[0x15C] = EKeyboardKeyName::RightSuper;
    KeyCodeFromScanCodeTable[0x15D] = EKeyboardKeyName::Menu;
    KeyCodeFromScanCodeTable[0x039] = EKeyboardKeyName::Space;
    KeyCodeFromScanCodeTable[0x028] = EKeyboardKeyName::Apostrophe;
    KeyCodeFromScanCodeTable[0x033] = EKeyboardKeyName::Comma;
    KeyCodeFromScanCodeTable[0x00C] = EKeyboardKeyName::Minus;
    KeyCodeFromScanCodeTable[0x034] = EKeyboardKeyName::Period;
    KeyCodeFromScanCodeTable[0x035] = EKeyboardKeyName::Slash;
    KeyCodeFromScanCodeTable[0x027] = EKeyboardKeyName::Semicolon;
    KeyCodeFromScanCodeTable[0x00D] = EKeyboardKeyName::Equal;
    KeyCodeFromScanCodeTable[0x01A] = EKeyboardKeyName::LeftBracket;
    KeyCodeFromScanCodeTable[0x02B] = EKeyboardKeyName::Backslash;
    KeyCodeFromScanCodeTable[0x01B] = EKeyboardKeyName::RightBracket;
    KeyCodeFromScanCodeTable[0x029] = EKeyboardKeyName::GraveAccent;
    KeyCodeFromScanCodeTable[0x056] = EKeyboardKeyName::World2;
    KeyCodeFromScanCodeTable[0x001] = EKeyboardKeyName::Escape;
    KeyCodeFromScanCodeTable[0x01C] = EKeyboardKeyName::Enter;
    KeyCodeFromScanCodeTable[0x00F] = EKeyboardKeyName::Tab;
    KeyCodeFromScanCodeTable[0x00E] = EKeyboardKeyName::Backspace;
    KeyCodeFromScanCodeTable[0x152] = EKeyboardKeyName::Insert;
    KeyCodeFromScanCodeTable[0x153] = EKeyboardKeyName::Delete;
    KeyCodeFromScanCodeTable[0x14D] = EKeyboardKeyName::Right;
    KeyCodeFromScanCodeTable[0x14B] = EKeyboardKeyName::Left;
    KeyCodeFromScanCodeTable[0x150] = EKeyboardKeyName::Down;
    KeyCodeFromScanCodeTable[0x148] = EKeyboardKeyName::Up;
    KeyCodeFromScanCodeTable[0x149] = EKeyboardKeyName::PageUp;
    KeyCodeFromScanCodeTable[0x151] = EKeyboardKeyName::PageDown;
    KeyCodeFromScanCodeTable[0x147] = EKeyboardKeyName::Home;
    KeyCodeFromScanCodeTable[0x14F] = EKeyboardKeyName::End;
    KeyCodeFromScanCodeTable[0x03A] = EKeyboardKeyName::CapsLock;
    KeyCodeFromScanCodeTable[0x046] = EKeyboardKeyName::ScrollLock;
    KeyCodeFromScanCodeTable[0x145] = EKeyboardKeyName::NumLock;
    KeyCodeFromScanCodeTable[0x137] = EKeyboardKeyName::PrintScreen;
    KeyCodeFromScanCodeTable[0x146] = EKeyboardKeyName::Pause;

    for (uint16 Index = 0; Index < NumKeys; ++Index)
    {
        if (KeyCodeFromScanCodeTable[Index] != EKeyboardKeyName::Unknown)
        {
            ScanCodeFromKeyCodeTable[KeyCodeFromScanCodeTable[Index]] = Index;
        }
    }
}