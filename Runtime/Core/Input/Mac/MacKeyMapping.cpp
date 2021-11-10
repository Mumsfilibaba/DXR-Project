#if PLATFORM_MACOS
#include "MacKeyMapping.h"

/* Keys */
TStaticArray<EKey, 256>   CMacKeyMapping::KeyCodeFromScanCodeTable;
TStaticArray<uint16, 256> CMacKeyMapping::ScanCodeFromKeyCodeTable;

/* Buttons */
TStaticArray<EMouseButton, EMouseButton::MouseButton_Count> CMacKeyMapping::ButtonFromButtonIndex;
TStaticArray<uint8, EMouseButton::MouseButton_Count>        CMacKeyMapping::ButtonIndexFromButton;

void CMacKeyMapping::Init()
{
    /* Keys */
    CMemory::Memzero( KeyCodeFromScanCodeTable.Data(), KeyCodeFromScanCodeTable.SizeInBytes() );
    CMemory::Memzero( ScanCodeFromKeyCodeTable.Data(), ScanCodeFromKeyCodeTable.SizeInBytes() );

    KeyCodeFromScanCodeTable[0x33] = EKey::Key_Backspace;
    KeyCodeFromScanCodeTable[0x30] = EKey::Key_Tab;
    KeyCodeFromScanCodeTable[0x24] = EKey::Key_Enter;
    KeyCodeFromScanCodeTable[0x39] = EKey::Key_CapsLock;
    KeyCodeFromScanCodeTable[0x31] = EKey::Key_Space;
    KeyCodeFromScanCodeTable[0x74] = EKey::Key_PageUp;
    KeyCodeFromScanCodeTable[0x79] = EKey::Key_PageDown;
    KeyCodeFromScanCodeTable[0x77] = EKey::Key_End;
    KeyCodeFromScanCodeTable[0x73] = EKey::Key_Home;
    KeyCodeFromScanCodeTable[0x7B] = EKey::Key_Left;
    KeyCodeFromScanCodeTable[0x7E] = EKey::Key_Up;
    KeyCodeFromScanCodeTable[0x7C] = EKey::Key_Right;
    KeyCodeFromScanCodeTable[0x7D] = EKey::Key_Down;
    KeyCodeFromScanCodeTable[0x72] = EKey::Key_Insert;
    KeyCodeFromScanCodeTable[0x75] = EKey::Key_Delete;
    KeyCodeFromScanCodeTable[0x35] = EKey::Key_Escape;
    KeyCodeFromScanCodeTable[0x1D] = EKey::Key_0;
    KeyCodeFromScanCodeTable[0x12] = EKey::Key_1;
    KeyCodeFromScanCodeTable[0x13] = EKey::Key_2;
    KeyCodeFromScanCodeTable[0x14] = EKey::Key_3;
    KeyCodeFromScanCodeTable[0x15] = EKey::Key_4;
    KeyCodeFromScanCodeTable[0x17] = EKey::Key_5;
    KeyCodeFromScanCodeTable[0x16] = EKey::Key_6;
    KeyCodeFromScanCodeTable[0x1A] = EKey::Key_7;
    KeyCodeFromScanCodeTable[0x1C] = EKey::Key_8;
    KeyCodeFromScanCodeTable[0x19] = EKey::Key_9;
    KeyCodeFromScanCodeTable[0x00] = EKey::Key_A;
    KeyCodeFromScanCodeTable[0x0B] = EKey::Key_B;
    KeyCodeFromScanCodeTable[0x08] = EKey::Key_C;
    KeyCodeFromScanCodeTable[0x02] = EKey::Key_D;
    KeyCodeFromScanCodeTable[0x0E] = EKey::Key_E;
    KeyCodeFromScanCodeTable[0x03] = EKey::Key_F;
    KeyCodeFromScanCodeTable[0x05] = EKey::Key_G;
    KeyCodeFromScanCodeTable[0x04] = EKey::Key_H;
    KeyCodeFromScanCodeTable[0x22] = EKey::Key_I;
    KeyCodeFromScanCodeTable[0x26] = EKey::Key_J;
    KeyCodeFromScanCodeTable[0x28] = EKey::Key_K;
    KeyCodeFromScanCodeTable[0x25] = EKey::Key_L;
    KeyCodeFromScanCodeTable[0x2E] = EKey::Key_M;
    KeyCodeFromScanCodeTable[0x2D] = EKey::Key_N;
    KeyCodeFromScanCodeTable[0x1F] = EKey::Key_O;
    KeyCodeFromScanCodeTable[0x23] = EKey::Key_P;
    KeyCodeFromScanCodeTable[0x0C] = EKey::Key_Q;
    KeyCodeFromScanCodeTable[0x0F] = EKey::Key_R;
    KeyCodeFromScanCodeTable[0x01] = EKey::Key_S;
    KeyCodeFromScanCodeTable[0x11] = EKey::Key_T;
    KeyCodeFromScanCodeTable[0x20] = EKey::Key_U;
    KeyCodeFromScanCodeTable[0x09] = EKey::Key_V;
    KeyCodeFromScanCodeTable[0x0D] = EKey::Key_W;
    KeyCodeFromScanCodeTable[0x07] = EKey::Key_X;
    KeyCodeFromScanCodeTable[0x10] = EKey::Key_Y;
    KeyCodeFromScanCodeTable[0x06] = EKey::Key_Z;
    KeyCodeFromScanCodeTable[0x52] = EKey::Key_Keypad0;
    KeyCodeFromScanCodeTable[0x53] = EKey::Key_Keypad1;
    KeyCodeFromScanCodeTable[0x54] = EKey::Key_Keypad2;
    KeyCodeFromScanCodeTable[0x55] = EKey::Key_Keypad3;
    KeyCodeFromScanCodeTable[0x56] = EKey::Key_Keypad4;
    KeyCodeFromScanCodeTable[0x57] = EKey::Key_Keypad5;
    KeyCodeFromScanCodeTable[0x58] = EKey::Key_Keypad6;
    KeyCodeFromScanCodeTable[0x59] = EKey::Key_Keypad7;
    KeyCodeFromScanCodeTable[0x5B] = EKey::Key_Keypad8;
    KeyCodeFromScanCodeTable[0x5C] = EKey::Key_Keypad9;
    KeyCodeFromScanCodeTable[0x45] = EKey::Key_KeypadAdd;
    KeyCodeFromScanCodeTable[0x41] = EKey::Key_KeypadDecimal;
    KeyCodeFromScanCodeTable[0x4B] = EKey::Key_KeypadDivide;
    KeyCodeFromScanCodeTable[0x4C] = EKey::Key_KeypadEnter;
    KeyCodeFromScanCodeTable[0x51] = EKey::Key_KeypadEqual;
    KeyCodeFromScanCodeTable[0x43] = EKey::Key_KeypadMultiply;
    KeyCodeFromScanCodeTable[0x4E] = EKey::Key_KeypadSubtract;
    KeyCodeFromScanCodeTable[0x7A] = EKey::Key_F1;
    KeyCodeFromScanCodeTable[0x78] = EKey::Key_F2;
    KeyCodeFromScanCodeTable[0x63] = EKey::Key_F3;
    KeyCodeFromScanCodeTable[0x76] = EKey::Key_F4;
    KeyCodeFromScanCodeTable[0x60] = EKey::Key_F5;
    KeyCodeFromScanCodeTable[0x61] = EKey::Key_F6;
    KeyCodeFromScanCodeTable[0x62] = EKey::Key_F7;
    KeyCodeFromScanCodeTable[0x64] = EKey::Key_F8;
    KeyCodeFromScanCodeTable[0x65] = EKey::Key_F9;
    KeyCodeFromScanCodeTable[0x6D] = EKey::Key_F10;
    KeyCodeFromScanCodeTable[0x67] = EKey::Key_F11;
    KeyCodeFromScanCodeTable[0x6F] = EKey::Key_F12;
    KeyCodeFromScanCodeTable[0x69] = EKey::Key_F13;
    KeyCodeFromScanCodeTable[0x6B] = EKey::Key_F14;
    KeyCodeFromScanCodeTable[0x71] = EKey::Key_F15;
    KeyCodeFromScanCodeTable[0x6A] = EKey::Key_F16;
    KeyCodeFromScanCodeTable[0x40] = EKey::Key_F17;
    KeyCodeFromScanCodeTable[0x4F] = EKey::Key_F18;
    KeyCodeFromScanCodeTable[0x50] = EKey::Key_F19;
    KeyCodeFromScanCodeTable[0x5A] = EKey::Key_F20;
    KeyCodeFromScanCodeTable[0x47] = EKey::Key_NumLock;
    KeyCodeFromScanCodeTable[0x29] = EKey::Key_Semicolon;
    KeyCodeFromScanCodeTable[0x2B] = EKey::Key_Comma;
    KeyCodeFromScanCodeTable[0x1B] = EKey::Key_Minus;
    KeyCodeFromScanCodeTable[0x2F] = EKey::Key_Period;
    KeyCodeFromScanCodeTable[0x32] = EKey::Key_GraveAccent;
    KeyCodeFromScanCodeTable[0x21] = EKey::Key_LeftBracket;
    KeyCodeFromScanCodeTable[0x1E] = EKey::Key_RightBracket;
    KeyCodeFromScanCodeTable[0x27] = EKey::Key_Apostrophe;
    KeyCodeFromScanCodeTable[0x2A] = EKey::Key_Backslash;
    KeyCodeFromScanCodeTable[0x38] = EKey::Key_LeftShift;
    KeyCodeFromScanCodeTable[0x3B] = EKey::Key_LeftControl;
    KeyCodeFromScanCodeTable[0x3A] = EKey::Key_LeftAlt;
    KeyCodeFromScanCodeTable[0x37] = EKey::Key_LeftSuper;
    KeyCodeFromScanCodeTable[0x3C] = EKey::Key_RightShift;
    KeyCodeFromScanCodeTable[0x3E] = EKey::Key_LeftControl;
    KeyCodeFromScanCodeTable[0x36] = EKey::Key_RightSuper;
    KeyCodeFromScanCodeTable[0x3D] = EKey::Key_LeftAlt;
    KeyCodeFromScanCodeTable[0x6E] = EKey::Key_Menu;
    KeyCodeFromScanCodeTable[0x18] = EKey::Key_Equal;
    KeyCodeFromScanCodeTable[0x2C] = EKey::Key_Slash;
    KeyCodeFromScanCodeTable[0x0A] = EKey::Key_World1;

    for ( uint16 Index = 0; Index < KeyCodeFromScanCodeTable.Size(); Index++ )
    {
        if ( KeyCodeFromScanCodeTable[Index] != EKey::Key_Unknown )
        {
            ScanCodeFromKeyCodeTable[KeyCodeFromScanCodeTable[Index]] = Index;
        }
    }

    /* Mouse buttons */
    CMemory::Memzero( ButtonFromButtonIndex.Data(), ButtonFromButtonIndex.SizeInBytes() );
    CMemory::Memzero( ButtonIndexFromButton.Data(), ButtonIndexFromButton.SizeInBytes() );

    ButtonFromButtonIndex[0] = EMouseButton::MouseButton_Left;
    ButtonFromButtonIndex[1] = EMouseButton::MouseButton_Right;
    ButtonFromButtonIndex[2] = EMouseButton::MouseButton_Middle;

    for ( uint8 Index = 0; Index < EMouseButton::MouseButton_Count; Index++ )
    {
        if ( ButtonFromButtonIndex[Index] != EMouseButton::MouseButton_Unknown )
        {
            ButtonIndexFromButton[ButtonFromButtonIndex[Index]] = Index;
        }
    }
}

#endif

