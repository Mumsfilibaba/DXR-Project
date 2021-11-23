#if PLATFORM_WINDOWS
#include "WindowsKeyMapping.h"

EKey   CWindowsKeyMapping::KeyCodeFromScanCodeTable[NumKeys];
uint16 CWindowsKeyMapping::ScanCodeFromKeyCodeTable[NumKeys];

///////////////////////////////////////////////////////////////////////////////////////////////////

void CWindowsKeyMapping::Init()
{
    CMemory::Memzero( KeyCodeFromScanCodeTable, sizeof(KeyCodeFromScanCodeTable) );
    CMemory::Memzero( ScanCodeFromKeyCodeTable, sizeof(ScanCodeFromKeyCodeTable) );

    KeyCodeFromScanCodeTable[0x00B] = EKey::Key_0;
    KeyCodeFromScanCodeTable[0x002] = EKey::Key_1;
    KeyCodeFromScanCodeTable[0x003] = EKey::Key_2;
    KeyCodeFromScanCodeTable[0x004] = EKey::Key_3;
    KeyCodeFromScanCodeTable[0x005] = EKey::Key_4;
    KeyCodeFromScanCodeTable[0x006] = EKey::Key_5;
    KeyCodeFromScanCodeTable[0x007] = EKey::Key_6;
    KeyCodeFromScanCodeTable[0x008] = EKey::Key_7;
    KeyCodeFromScanCodeTable[0x009] = EKey::Key_8;
    KeyCodeFromScanCodeTable[0x00A] = EKey::Key_9;

    KeyCodeFromScanCodeTable[0x01E] = EKey::Key_A;
    KeyCodeFromScanCodeTable[0x030] = EKey::Key_B;
    KeyCodeFromScanCodeTable[0x02E] = EKey::Key_C;
    KeyCodeFromScanCodeTable[0x020] = EKey::Key_D;
    KeyCodeFromScanCodeTable[0x012] = EKey::Key_E;
    KeyCodeFromScanCodeTable[0x021] = EKey::Key_F;
    KeyCodeFromScanCodeTable[0x022] = EKey::Key_G;
    KeyCodeFromScanCodeTable[0x023] = EKey::Key_H;
    KeyCodeFromScanCodeTable[0x017] = EKey::Key_I;
    KeyCodeFromScanCodeTable[0x024] = EKey::Key_J;
    KeyCodeFromScanCodeTable[0x025] = EKey::Key_K;
    KeyCodeFromScanCodeTable[0x026] = EKey::Key_L;
    KeyCodeFromScanCodeTable[0x032] = EKey::Key_M;
    KeyCodeFromScanCodeTable[0x031] = EKey::Key_N;
    KeyCodeFromScanCodeTable[0x018] = EKey::Key_O;
    KeyCodeFromScanCodeTable[0x019] = EKey::Key_P;
    KeyCodeFromScanCodeTable[0x010] = EKey::Key_Q;
    KeyCodeFromScanCodeTable[0x013] = EKey::Key_R;
    KeyCodeFromScanCodeTable[0x01F] = EKey::Key_S;
    KeyCodeFromScanCodeTable[0x014] = EKey::Key_T;
    KeyCodeFromScanCodeTable[0x016] = EKey::Key_U;
    KeyCodeFromScanCodeTable[0x02F] = EKey::Key_V;
    KeyCodeFromScanCodeTable[0x011] = EKey::Key_W;
    KeyCodeFromScanCodeTable[0x02D] = EKey::Key_X;
    KeyCodeFromScanCodeTable[0x015] = EKey::Key_Y;
    KeyCodeFromScanCodeTable[0x02C] = EKey::Key_Z;

    KeyCodeFromScanCodeTable[0x03B] = EKey::Key_F1;
    KeyCodeFromScanCodeTable[0x03C] = EKey::Key_F2;
    KeyCodeFromScanCodeTable[0x03D] = EKey::Key_F3;
    KeyCodeFromScanCodeTable[0x03E] = EKey::Key_F4;
    KeyCodeFromScanCodeTable[0x03F] = EKey::Key_F5;
    KeyCodeFromScanCodeTable[0x040] = EKey::Key_F6;
    KeyCodeFromScanCodeTable[0x041] = EKey::Key_F7;
    KeyCodeFromScanCodeTable[0x042] = EKey::Key_F8;
    KeyCodeFromScanCodeTable[0x043] = EKey::Key_F9;
    KeyCodeFromScanCodeTable[0x044] = EKey::Key_F10;
    KeyCodeFromScanCodeTable[0x057] = EKey::Key_F11;
    KeyCodeFromScanCodeTable[0x058] = EKey::Key_F12;
    KeyCodeFromScanCodeTable[0x064] = EKey::Key_F13;
    KeyCodeFromScanCodeTable[0x065] = EKey::Key_F14;
    KeyCodeFromScanCodeTable[0x066] = EKey::Key_F15;
    KeyCodeFromScanCodeTable[0x067] = EKey::Key_F16;
    KeyCodeFromScanCodeTable[0x068] = EKey::Key_F17;
    KeyCodeFromScanCodeTable[0x069] = EKey::Key_F18;
    KeyCodeFromScanCodeTable[0x06A] = EKey::Key_F19;
    KeyCodeFromScanCodeTable[0x06B] = EKey::Key_F20;
    KeyCodeFromScanCodeTable[0x06C] = EKey::Key_F21;
    KeyCodeFromScanCodeTable[0x06D] = EKey::Key_F22;
    KeyCodeFromScanCodeTable[0x06E] = EKey::Key_F23;
    KeyCodeFromScanCodeTable[0x076] = EKey::Key_F24;

    KeyCodeFromScanCodeTable[0x052] = EKey::Key_Keypad0;
    KeyCodeFromScanCodeTable[0x04F] = EKey::Key_Keypad1;
    KeyCodeFromScanCodeTable[0x050] = EKey::Key_Keypad2;
    KeyCodeFromScanCodeTable[0x051] = EKey::Key_Keypad3;
    KeyCodeFromScanCodeTable[0x04B] = EKey::Key_Keypad4;
    KeyCodeFromScanCodeTable[0x04C] = EKey::Key_Keypad5;
    KeyCodeFromScanCodeTable[0x04D] = EKey::Key_Keypad6;
    KeyCodeFromScanCodeTable[0x047] = EKey::Key_Keypad7;
    KeyCodeFromScanCodeTable[0x048] = EKey::Key_Keypad8;
    KeyCodeFromScanCodeTable[0x049] = EKey::Key_Keypad9;
    KeyCodeFromScanCodeTable[0x04E] = EKey::Key_KeypadAdd;
    KeyCodeFromScanCodeTable[0x053] = EKey::Key_KeypadDecimal;
    KeyCodeFromScanCodeTable[0x135] = EKey::Key_KeypadDivide;
    KeyCodeFromScanCodeTable[0x11C] = EKey::Key_KeypadEnter;
    KeyCodeFromScanCodeTable[0x059] = EKey::Key_KeypadEqual;
    KeyCodeFromScanCodeTable[0x037] = EKey::Key_KeypadMultiply;
    KeyCodeFromScanCodeTable[0x04A] = EKey::Key_KeypadSubtract;

    KeyCodeFromScanCodeTable[0x02A] = EKey::Key_LeftShift;
    KeyCodeFromScanCodeTable[0x036] = EKey::Key_RightShift;
    KeyCodeFromScanCodeTable[0x01D] = EKey::Key_LeftControl;
    KeyCodeFromScanCodeTable[0x11D] = EKey::Key_RightControl;
    KeyCodeFromScanCodeTable[0x038] = EKey::Key_LeftAlt;
    KeyCodeFromScanCodeTable[0x138] = EKey::Key_RightAlt;
    KeyCodeFromScanCodeTable[0x15B] = EKey::Key_LeftSuper;
    KeyCodeFromScanCodeTable[0x15C] = EKey::Key_RightSuper;
    KeyCodeFromScanCodeTable[0x15D] = EKey::Key_Menu;
    KeyCodeFromScanCodeTable[0x039] = EKey::Key_Space;
    KeyCodeFromScanCodeTable[0x028] = EKey::Key_Apostrophe;
    KeyCodeFromScanCodeTable[0x033] = EKey::Key_Comma;
    KeyCodeFromScanCodeTable[0x00C] = EKey::Key_Minus;
    KeyCodeFromScanCodeTable[0x034] = EKey::Key_Period;
    KeyCodeFromScanCodeTable[0x035] = EKey::Key_Slash;
    KeyCodeFromScanCodeTable[0x027] = EKey::Key_Semicolon;
    KeyCodeFromScanCodeTable[0x00D] = EKey::Key_Equal;
    KeyCodeFromScanCodeTable[0x01A] = EKey::Key_LeftBracket;
    KeyCodeFromScanCodeTable[0x02B] = EKey::Key_Backslash;
    KeyCodeFromScanCodeTable[0x01B] = EKey::Key_RightBracket;
    KeyCodeFromScanCodeTable[0x029] = EKey::Key_GraveAccent;
    KeyCodeFromScanCodeTable[0x056] = EKey::Key_World2;
    KeyCodeFromScanCodeTable[0x001] = EKey::Key_Escape;
    KeyCodeFromScanCodeTable[0x01C] = EKey::Key_Enter;
    KeyCodeFromScanCodeTable[0x00F] = EKey::Key_Tab;
    KeyCodeFromScanCodeTable[0x00E] = EKey::Key_Backspace;
    KeyCodeFromScanCodeTable[0x152] = EKey::Key_Insert;
    KeyCodeFromScanCodeTable[0x153] = EKey::Key_Delete;
    KeyCodeFromScanCodeTable[0x14D] = EKey::Key_Right;
    KeyCodeFromScanCodeTable[0x14B] = EKey::Key_Left;
    KeyCodeFromScanCodeTable[0x150] = EKey::Key_Down;
    KeyCodeFromScanCodeTable[0x148] = EKey::Key_Up;
    KeyCodeFromScanCodeTable[0x149] = EKey::Key_PageUp;
    KeyCodeFromScanCodeTable[0x151] = EKey::Key_PageDown;
    KeyCodeFromScanCodeTable[0x147] = EKey::Key_Home;
    KeyCodeFromScanCodeTable[0x14F] = EKey::Key_End;
    KeyCodeFromScanCodeTable[0x03A] = EKey::Key_CapsLock;
    KeyCodeFromScanCodeTable[0x046] = EKey::Key_ScrollLock;
    KeyCodeFromScanCodeTable[0x145] = EKey::Key_NumLock;
    KeyCodeFromScanCodeTable[0x137] = EKey::Key_PrintScreen;
    KeyCodeFromScanCodeTable[0x146] = EKey::Key_Pause;

    for ( uint16 Index = 0; Index < KeyCodeFromScanCodeTable.Size(); Index++ )
    {
        if ( KeyCodeFromScanCodeTable[Index] != EKey::Key_Unknown )
        {
            ScanCodeFromKeyCodeTable[KeyCodeFromScanCodeTable[Index]] = Index;
        }
    }
}

#endif