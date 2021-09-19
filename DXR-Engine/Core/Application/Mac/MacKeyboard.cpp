#if defined(PLATFORM_MACOS)
#include "Core/Memory/Memory.h"

#include "MacKeyboard.h"

bool CMacKeyboard::InitKeyTables()
{
	Memory::Memzero(KeyCodeFromScanCode, sizeof(KeyCodeFromScanCode));
    Memory::Memzero(KeyState, sizeof(KeyState));
	
	KeyCodeFromScanCode[0x33] = EKey::Key_Backspace;
	KeyCodeFromScanCode[0x30] = EKey::Key_Tab;
	KeyCodeFromScanCode[0x24] = EKey::Key_Enter;
	KeyCodeFromScanCode[0x39] = EKey::Key_CapsLock;
	KeyCodeFromScanCode[0x31] = EKey::Key_Space;
	KeyCodeFromScanCode[0x74] = EKey::Key_PageUp;
	KeyCodeFromScanCode[0x79] = EKey::Key_PageDown;
	KeyCodeFromScanCode[0x77] = EKey::Key_End;
	KeyCodeFromScanCode[0x73] = EKey::Key_Home;
	KeyCodeFromScanCode[0x7B] = EKey::Key_Left;
	KeyCodeFromScanCode[0x7E] = EKey::Key_Up;
	KeyCodeFromScanCode[0x7C] = EKey::Key_Right;
	KeyCodeFromScanCode[0x7D] = EKey::Key_Down;
	KeyCodeFromScanCode[0x72] = EKey::Key_Insert;
	KeyCodeFromScanCode[0x75] = EKey::Key_Delete;
	KeyCodeFromScanCode[0x35] = EKey::Key_Escape;
	KeyCodeFromScanCode[0x1D] = EKey::Key_0;
	KeyCodeFromScanCode[0x12] = EKey::Key_1;
	KeyCodeFromScanCode[0x13] = EKey::Key_2;
	KeyCodeFromScanCode[0x14] = EKey::Key_3;
	KeyCodeFromScanCode[0x15] = EKey::Key_4;
	KeyCodeFromScanCode[0x17] = EKey::Key_5;
	KeyCodeFromScanCode[0x16] = EKey::Key_6;
	KeyCodeFromScanCode[0x1A] = EKey::Key_7;
	KeyCodeFromScanCode[0x1C] = EKey::Key_8;
	KeyCodeFromScanCode[0x19] = EKey::Key_9;
	KeyCodeFromScanCode[0x00] = EKey::Key_A;
	KeyCodeFromScanCode[0x0B] = EKey::Key_B;
	KeyCodeFromScanCode[0x08] = EKey::Key_C;
	KeyCodeFromScanCode[0x02] = EKey::Key_D;
	KeyCodeFromScanCode[0x0E] = EKey::Key_E;
	KeyCodeFromScanCode[0x03] = EKey::Key_F;
	KeyCodeFromScanCode[0x05] = EKey::Key_G;
	KeyCodeFromScanCode[0x04] = EKey::Key_H;
	KeyCodeFromScanCode[0x22] = EKey::Key_I;
	KeyCodeFromScanCode[0x26] = EKey::Key_J;
	KeyCodeFromScanCode[0x28] = EKey::Key_K;
	KeyCodeFromScanCode[0x25] = EKey::Key_L;
	KeyCodeFromScanCode[0x2E] = EKey::Key_M;
	KeyCodeFromScanCode[0x2D] = EKey::Key_N;
	KeyCodeFromScanCode[0x1F] = EKey::Key_O;
	KeyCodeFromScanCode[0x23] = EKey::Key_P;
	KeyCodeFromScanCode[0x0C] = EKey::Key_Q;
	KeyCodeFromScanCode[0x0F] = EKey::Key_R;
	KeyCodeFromScanCode[0x01] = EKey::Key_S;
	KeyCodeFromScanCode[0x11] = EKey::Key_T;
	KeyCodeFromScanCode[0x20] = EKey::Key_U;
	KeyCodeFromScanCode[0x09] = EKey::Key_V;
	KeyCodeFromScanCode[0x0D] = EKey::Key_W;
	KeyCodeFromScanCode[0x07] = EKey::Key_X;
	KeyCodeFromScanCode[0x10] = EKey::Key_Y;
	KeyCodeFromScanCode[0x06] = EKey::Key_Z;
	KeyCodeFromScanCode[0x52] = EKey::Key_Keypad0;
	KeyCodeFromScanCode[0x53] = EKey::Key_Keypad1;
	KeyCodeFromScanCode[0x54] = EKey::Key_Keypad2;
	KeyCodeFromScanCode[0x55] = EKey::Key_Keypad3;
	KeyCodeFromScanCode[0x56] = EKey::Key_Keypad4;
	KeyCodeFromScanCode[0x57] = EKey::Key_Keypad5;
	KeyCodeFromScanCode[0x58] = EKey::Key_Keypad6;
	KeyCodeFromScanCode[0x59] = EKey::Key_Keypad7;
	KeyCodeFromScanCode[0x5B] = EKey::Key_Keypad8;
	KeyCodeFromScanCode[0x5C] = EKey::Key_Keypad9;
	KeyCodeFromScanCode[0x45] = EKey::Key_KeypadAdd;
	KeyCodeFromScanCode[0x41] = EKey::Key_KeypadDecimal;
	KeyCodeFromScanCode[0x4B] = EKey::Key_KeypadDivide;
	KeyCodeFromScanCode[0x4C] = EKey::Key_KeypadEnter;
	KeyCodeFromScanCode[0x51] = EKey::Key_KeypadEqual;
	KeyCodeFromScanCode[0x43] = EKey::Key_KeypadMultiply;
	KeyCodeFromScanCode[0x4E] = EKey::Key_KeypadSubtract;
	KeyCodeFromScanCode[0x7A] = EKey::Key_F1;
	KeyCodeFromScanCode[0x78] = EKey::Key_F2;
	KeyCodeFromScanCode[0x63] = EKey::Key_F3;
	KeyCodeFromScanCode[0x76] = EKey::Key_F4;
	KeyCodeFromScanCode[0x60] = EKey::Key_F5;
	KeyCodeFromScanCode[0x61] = EKey::Key_F6;
	KeyCodeFromScanCode[0x62] = EKey::Key_F7;
	KeyCodeFromScanCode[0x64] = EKey::Key_F8;
	KeyCodeFromScanCode[0x65] = EKey::Key_F9;
	KeyCodeFromScanCode[0x6D] = EKey::Key_F10;
	KeyCodeFromScanCode[0x67] = EKey::Key_F11;
	KeyCodeFromScanCode[0x6F] = EKey::Key_F12;
	KeyCodeFromScanCode[0x69] = EKey::Key_F13;
	KeyCodeFromScanCode[0x6B] = EKey::Key_F14;
	KeyCodeFromScanCode[0x71] = EKey::Key_F15;
	KeyCodeFromScanCode[0x6A] = EKey::Key_F16;
	KeyCodeFromScanCode[0x40] = EKey::Key_F17;
	KeyCodeFromScanCode[0x4F] = EKey::Key_F18;
	KeyCodeFromScanCode[0x50] = EKey::Key_F19;
	KeyCodeFromScanCode[0x5A] = EKey::Key_F20;
	KeyCodeFromScanCode[0x47] = EKey::Key_NumLock;
	KeyCodeFromScanCode[0x29] = EKey::Key_Semicolon;
	KeyCodeFromScanCode[0x2B] = EKey::Key_Comma;
	KeyCodeFromScanCode[0x1B] = EKey::Key_Minus;
	KeyCodeFromScanCode[0x2F] = EKey::Key_Period;
	KeyCodeFromScanCode[0x32] = EKey::Key_GraveAccent;
	KeyCodeFromScanCode[0x21] = EKey::Key_LeftBracket;
	KeyCodeFromScanCode[0x1E] = EKey::Key_RightBracket;
	KeyCodeFromScanCode[0x27] = EKey::Key_Apostrophe;
	KeyCodeFromScanCode[0x2A] = EKey::Key_Backslash;
	KeyCodeFromScanCode[0x38] = EKey::Key_LeftShift;
	KeyCodeFromScanCode[0x3B] = EKey::Key_LeftControl;
	KeyCodeFromScanCode[0x3A] = EKey::Key_LeftAlt;
	KeyCodeFromScanCode[0x37] = EKey::Key_LeftSuper;
	KeyCodeFromScanCode[0x3C] = EKey::Key_RightShift;
	KeyCodeFromScanCode[0x3E] = EKey::Key_LeftControl;
	KeyCodeFromScanCode[0x36] = EKey::Key_RightSuper;
	KeyCodeFromScanCode[0x3D] = EKey::Key_LeftAlt;
	KeyCodeFromScanCode[0x6E] = EKey::Key_Menu;
	KeyCodeFromScanCode[0x18] = EKey::Key_Equal;
	KeyCodeFromScanCode[0x2C] = EKey::Key_Slash;
	KeyCodeFromScanCode[0x0A] = EKey::Key_World1;
	
	return true;
}

bool CMacKeyboard::IsKeyDown( EKey KeyCode ) const
{
	return KeyState[KeyCode];
}

bool CMacKeyboard::IsKeyUp( EKey KeyCode ) const
{
	return !KeyState[KeyCode];
}

#endif
