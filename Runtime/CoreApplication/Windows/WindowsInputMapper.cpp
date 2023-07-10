#include "WindowsInputMapper.h"

TStaticArray<uint16, FWindowsInputMapper::NumKeys> FWindowsInputMapper::ScanCodeFromKeyCodeTable;
TStaticArray<EKeyName::Type  , FWindowsInputMapper::NumKeys> FWindowsInputMapper::KeyCodeFromScanCodeTable;

void FWindowsInputMapper::Initialize()
{
    KeyCodeFromScanCodeTable.Memzero(); 
    ScanCodeFromKeyCodeTable.Memzero();

    KeyCodeFromScanCodeTable[0x00B] = EKeyName::Zero;
    KeyCodeFromScanCodeTable[0x002] = EKeyName::One;
    KeyCodeFromScanCodeTable[0x003] = EKeyName::Two;
    KeyCodeFromScanCodeTable[0x004] = EKeyName::Three;
    KeyCodeFromScanCodeTable[0x005] = EKeyName::Four;
    KeyCodeFromScanCodeTable[0x006] = EKeyName::Five;
    KeyCodeFromScanCodeTable[0x007] = EKeyName::Six;
    KeyCodeFromScanCodeTable[0x008] = EKeyName::Seven;
    KeyCodeFromScanCodeTable[0x009] = EKeyName::Eight;
    KeyCodeFromScanCodeTable[0x00A] = EKeyName::Nine;

    KeyCodeFromScanCodeTable[0x01E] = EKeyName::A;
    KeyCodeFromScanCodeTable[0x030] = EKeyName::B;
    KeyCodeFromScanCodeTable[0x02E] = EKeyName::C;
    KeyCodeFromScanCodeTable[0x020] = EKeyName::D;
    KeyCodeFromScanCodeTable[0x012] = EKeyName::E;
    KeyCodeFromScanCodeTable[0x021] = EKeyName::F;
    KeyCodeFromScanCodeTable[0x022] = EKeyName::G;
    KeyCodeFromScanCodeTable[0x023] = EKeyName::H;
    KeyCodeFromScanCodeTable[0x017] = EKeyName::I;
    KeyCodeFromScanCodeTable[0x024] = EKeyName::J;
    KeyCodeFromScanCodeTable[0x025] = EKeyName::K;
    KeyCodeFromScanCodeTable[0x026] = EKeyName::L;
    KeyCodeFromScanCodeTable[0x032] = EKeyName::M;
    KeyCodeFromScanCodeTable[0x031] = EKeyName::N;
    KeyCodeFromScanCodeTable[0x018] = EKeyName::O;
    KeyCodeFromScanCodeTable[0x019] = EKeyName::P;
    KeyCodeFromScanCodeTable[0x010] = EKeyName::Q;
    KeyCodeFromScanCodeTable[0x013] = EKeyName::R;
    KeyCodeFromScanCodeTable[0x01F] = EKeyName::S;
    KeyCodeFromScanCodeTable[0x014] = EKeyName::T;
    KeyCodeFromScanCodeTable[0x016] = EKeyName::U;
    KeyCodeFromScanCodeTable[0x02F] = EKeyName::V;
    KeyCodeFromScanCodeTable[0x011] = EKeyName::W;
    KeyCodeFromScanCodeTable[0x02D] = EKeyName::X;
    KeyCodeFromScanCodeTable[0x015] = EKeyName::Y;
    KeyCodeFromScanCodeTable[0x02C] = EKeyName::Z;

    KeyCodeFromScanCodeTable[0x03B] = EKeyName::F1;
    KeyCodeFromScanCodeTable[0x03C] = EKeyName::F2;
    KeyCodeFromScanCodeTable[0x03D] = EKeyName::F3;
    KeyCodeFromScanCodeTable[0x03E] = EKeyName::F4;
    KeyCodeFromScanCodeTable[0x03F] = EKeyName::F5;
    KeyCodeFromScanCodeTable[0x040] = EKeyName::F6;
    KeyCodeFromScanCodeTable[0x041] = EKeyName::F7;
    KeyCodeFromScanCodeTable[0x042] = EKeyName::F8;
    KeyCodeFromScanCodeTable[0x043] = EKeyName::F9;
    KeyCodeFromScanCodeTable[0x044] = EKeyName::F10;
    KeyCodeFromScanCodeTable[0x057] = EKeyName::F11;
    KeyCodeFromScanCodeTable[0x058] = EKeyName::F12;
    KeyCodeFromScanCodeTable[0x064] = EKeyName::F13;
    KeyCodeFromScanCodeTable[0x065] = EKeyName::F14;
    KeyCodeFromScanCodeTable[0x066] = EKeyName::F15;
    KeyCodeFromScanCodeTable[0x067] = EKeyName::F16;
    KeyCodeFromScanCodeTable[0x068] = EKeyName::F17;
    KeyCodeFromScanCodeTable[0x069] = EKeyName::F18;
    KeyCodeFromScanCodeTable[0x06A] = EKeyName::F19;
    KeyCodeFromScanCodeTable[0x06B] = EKeyName::F20;
    KeyCodeFromScanCodeTable[0x06C] = EKeyName::F21;
    KeyCodeFromScanCodeTable[0x06D] = EKeyName::F22;
    KeyCodeFromScanCodeTable[0x06E] = EKeyName::F23;
    KeyCodeFromScanCodeTable[0x076] = EKeyName::F24;

    KeyCodeFromScanCodeTable[0x052] = EKeyName::KeypadZero;
    KeyCodeFromScanCodeTable[0x04F] = EKeyName::KeypadOne;
    KeyCodeFromScanCodeTable[0x050] = EKeyName::KeypadTwo;
    KeyCodeFromScanCodeTable[0x051] = EKeyName::KeypadThree;
    KeyCodeFromScanCodeTable[0x04B] = EKeyName::KeypadFour;
    KeyCodeFromScanCodeTable[0x04C] = EKeyName::KeypadFive;
    KeyCodeFromScanCodeTable[0x04D] = EKeyName::KeypadSix;
    KeyCodeFromScanCodeTable[0x047] = EKeyName::KeypadSeven;
    KeyCodeFromScanCodeTable[0x048] = EKeyName::KeypadEight;
    KeyCodeFromScanCodeTable[0x049] = EKeyName::KeypadNine;
    KeyCodeFromScanCodeTable[0x04E] = EKeyName::KeypadAdd;
    KeyCodeFromScanCodeTable[0x053] = EKeyName::KeypadDecimal;
    KeyCodeFromScanCodeTable[0x135] = EKeyName::KeypadDivide;
    KeyCodeFromScanCodeTable[0x11C] = EKeyName::KeypadEnter;
    KeyCodeFromScanCodeTable[0x059] = EKeyName::KeypadEqual;
    KeyCodeFromScanCodeTable[0x037] = EKeyName::KeypadMultiply;
    KeyCodeFromScanCodeTable[0x04A] = EKeyName::KeypadSubtract;

    KeyCodeFromScanCodeTable[0x02A] = EKeyName::LeftShift;
    KeyCodeFromScanCodeTable[0x036] = EKeyName::RightShift;
    KeyCodeFromScanCodeTable[0x01D] = EKeyName::LeftControl;
    KeyCodeFromScanCodeTable[0x11D] = EKeyName::RightControl;
    KeyCodeFromScanCodeTable[0x038] = EKeyName::LeftAlt;
    KeyCodeFromScanCodeTable[0x138] = EKeyName::RightAlt;
    KeyCodeFromScanCodeTable[0x15B] = EKeyName::LeftSuper;
    KeyCodeFromScanCodeTable[0x15C] = EKeyName::RightSuper;
    KeyCodeFromScanCodeTable[0x15D] = EKeyName::Menu;
    KeyCodeFromScanCodeTable[0x039] = EKeyName::Space;
    KeyCodeFromScanCodeTable[0x028] = EKeyName::Apostrophe;
    KeyCodeFromScanCodeTable[0x033] = EKeyName::Comma;
    KeyCodeFromScanCodeTable[0x00C] = EKeyName::Minus;
    KeyCodeFromScanCodeTable[0x034] = EKeyName::Period;
    KeyCodeFromScanCodeTable[0x035] = EKeyName::Slash;
    KeyCodeFromScanCodeTable[0x027] = EKeyName::Semicolon;
    KeyCodeFromScanCodeTable[0x00D] = EKeyName::Equal;
    KeyCodeFromScanCodeTable[0x01A] = EKeyName::LeftBracket;
    KeyCodeFromScanCodeTable[0x02B] = EKeyName::Backslash;
    KeyCodeFromScanCodeTable[0x01B] = EKeyName::RightBracket;
    KeyCodeFromScanCodeTable[0x029] = EKeyName::GraveAccent;
    KeyCodeFromScanCodeTable[0x056] = EKeyName::World2;
    KeyCodeFromScanCodeTable[0x001] = EKeyName::Escape;
    KeyCodeFromScanCodeTable[0x01C] = EKeyName::Enter;
    KeyCodeFromScanCodeTable[0x00F] = EKeyName::Tab;
    KeyCodeFromScanCodeTable[0x00E] = EKeyName::Backspace;
    KeyCodeFromScanCodeTable[0x152] = EKeyName::Insert;
    KeyCodeFromScanCodeTable[0x153] = EKeyName::Delete;
    KeyCodeFromScanCodeTable[0x14D] = EKeyName::Right;
    KeyCodeFromScanCodeTable[0x14B] = EKeyName::Left;
    KeyCodeFromScanCodeTable[0x150] = EKeyName::Down;
    KeyCodeFromScanCodeTable[0x148] = EKeyName::Up;
    KeyCodeFromScanCodeTable[0x149] = EKeyName::PageUp;
    KeyCodeFromScanCodeTable[0x151] = EKeyName::PageDown;
    KeyCodeFromScanCodeTable[0x147] = EKeyName::Home;
    KeyCodeFromScanCodeTable[0x14F] = EKeyName::End;
    KeyCodeFromScanCodeTable[0x03A] = EKeyName::CapsLock;
    KeyCodeFromScanCodeTable[0x046] = EKeyName::ScrollLock;
    KeyCodeFromScanCodeTable[0x145] = EKeyName::NumLock;
    KeyCodeFromScanCodeTable[0x137] = EKeyName::PrintScreen;
    KeyCodeFromScanCodeTable[0x146] = EKeyName::Pause;

    for (uint16 Index = 0; Index < NumKeys; ++Index)
    {
        if (KeyCodeFromScanCodeTable[Index] != EKeyName::Unknown)
        {
            ScanCodeFromKeyCodeTable[KeyCodeFromScanCodeTable[Index]] = Index;
        }
    }
}