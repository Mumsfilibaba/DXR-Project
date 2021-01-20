#include "Input.h"

/*
* KeyTableState
*/

struct KeyTableState
{
	Bool	KeyStates[EKey::Key_Count];
	EKey	ScanCodeTable[512];
	UInt16	KeyTable[512];
	Bool	IsInitialized = false;
};

static KeyTableState GlobalkeyState;

/*
* Input
*/

static void InitializeKeyState()
{
	Memory::Memzero(GlobalkeyState.KeyStates, sizeof(GlobalkeyState.KeyStates));
	Memory::Memzero(GlobalkeyState.ScanCodeTable, sizeof(GlobalkeyState.ScanCodeTable));
	Memory::Memzero(GlobalkeyState.KeyTable, sizeof(GlobalkeyState.KeyTable));

	GlobalkeyState.ScanCodeTable[0x00B] = EKey::Key_0;
	GlobalkeyState.ScanCodeTable[0x002] = EKey::Key_1;
	GlobalkeyState.ScanCodeTable[0x003] = EKey::Key_2;
	GlobalkeyState.ScanCodeTable[0x004] = EKey::Key_3;
	GlobalkeyState.ScanCodeTable[0x005] = EKey::Key_4;
	GlobalkeyState.ScanCodeTable[0x006] = EKey::Key_5;
	GlobalkeyState.ScanCodeTable[0x007] = EKey::Key_6;
	GlobalkeyState.ScanCodeTable[0x008] = EKey::Key_7;
	GlobalkeyState.ScanCodeTable[0x009] = EKey::Key_8;
	GlobalkeyState.ScanCodeTable[0x00A] = EKey::Key_9;

	GlobalkeyState.ScanCodeTable[0x01E] = EKey::Key_A;
	GlobalkeyState.ScanCodeTable[0x030] = EKey::Key_B;
	GlobalkeyState.ScanCodeTable[0x02E] = EKey::Key_C;
	GlobalkeyState.ScanCodeTable[0x020] = EKey::Key_D;
	GlobalkeyState.ScanCodeTable[0x012] = EKey::Key_E;
	GlobalkeyState.ScanCodeTable[0x021] = EKey::Key_F;
	GlobalkeyState.ScanCodeTable[0x022] = EKey::Key_G;
	GlobalkeyState.ScanCodeTable[0x023] = EKey::Key_H;
	GlobalkeyState.ScanCodeTable[0x017] = EKey::Key_I;
	GlobalkeyState.ScanCodeTable[0x024] = EKey::Key_J;
	GlobalkeyState.ScanCodeTable[0x025] = EKey::Key_K;
	GlobalkeyState.ScanCodeTable[0x026] = EKey::Key_L;
	GlobalkeyState.ScanCodeTable[0x032] = EKey::Key_M;
	GlobalkeyState.ScanCodeTable[0x031] = EKey::Key_N;
	GlobalkeyState.ScanCodeTable[0x018] = EKey::Key_O;
	GlobalkeyState.ScanCodeTable[0x019] = EKey::Key_P;
	GlobalkeyState.ScanCodeTable[0x010] = EKey::Key_Q;
	GlobalkeyState.ScanCodeTable[0x013] = EKey::Key_R;
	GlobalkeyState.ScanCodeTable[0x01F] = EKey::Key_S;
	GlobalkeyState.ScanCodeTable[0x014] = EKey::Key_T;
	GlobalkeyState.ScanCodeTable[0x016] = EKey::Key_U;
	GlobalkeyState.ScanCodeTable[0x02F] = EKey::Key_V;
	GlobalkeyState.ScanCodeTable[0x011] = EKey::Key_W;
	GlobalkeyState.ScanCodeTable[0x02D] = EKey::Key_X;
	GlobalkeyState.ScanCodeTable[0x015] = EKey::Key_Y;
	GlobalkeyState.ScanCodeTable[0x02C] = EKey::Key_Z;

	GlobalkeyState.ScanCodeTable[0x03B] = EKey::Key_F1;
	GlobalkeyState.ScanCodeTable[0x03C] = EKey::Key_F2;
	GlobalkeyState.ScanCodeTable[0x03D] = EKey::Key_F3;
	GlobalkeyState.ScanCodeTable[0x03E] = EKey::Key_F4;
	GlobalkeyState.ScanCodeTable[0x03F] = EKey::Key_F5;
	GlobalkeyState.ScanCodeTable[0x040] = EKey::Key_F6;
	GlobalkeyState.ScanCodeTable[0x041] = EKey::Key_F7;
	GlobalkeyState.ScanCodeTable[0x042] = EKey::Key_F8;
	GlobalkeyState.ScanCodeTable[0x043] = EKey::Key_F9;
	GlobalkeyState.ScanCodeTable[0x044] = EKey::Key_F10;
	GlobalkeyState.ScanCodeTable[0x057] = EKey::Key_F11;
	GlobalkeyState.ScanCodeTable[0x058] = EKey::Key_F12;
	GlobalkeyState.ScanCodeTable[0x064] = EKey::Key_F13;
	GlobalkeyState.ScanCodeTable[0x065] = EKey::Key_F14;
	GlobalkeyState.ScanCodeTable[0x066] = EKey::Key_F15;
	GlobalkeyState.ScanCodeTable[0x067] = EKey::Key_F16;
	GlobalkeyState.ScanCodeTable[0x068] = EKey::Key_F17;
	GlobalkeyState.ScanCodeTable[0x069] = EKey::Key_F18;
	GlobalkeyState.ScanCodeTable[0x06A] = EKey::Key_F19;
	GlobalkeyState.ScanCodeTable[0x06B] = EKey::Key_F20;
	GlobalkeyState.ScanCodeTable[0x06C] = EKey::Key_F21;
	GlobalkeyState.ScanCodeTable[0x06D] = EKey::Key_F22;
	GlobalkeyState.ScanCodeTable[0x06E] = EKey::Key_F23;
	GlobalkeyState.ScanCodeTable[0x076] = EKey::Key_F24;

	GlobalkeyState.ScanCodeTable[0x052] = EKey::Key_Keypad0;
	GlobalkeyState.ScanCodeTable[0x04F] = EKey::Key_Keypad1;
	GlobalkeyState.ScanCodeTable[0x050] = EKey::Key_Keypad2;
	GlobalkeyState.ScanCodeTable[0x051] = EKey::Key_Keypad3;
	GlobalkeyState.ScanCodeTable[0x04B] = EKey::Key_Keypad4;
	GlobalkeyState.ScanCodeTable[0x04C] = EKey::Key_Keypad5;
	GlobalkeyState.ScanCodeTable[0x04D] = EKey::Key_Keypad6;
	GlobalkeyState.ScanCodeTable[0x047] = EKey::Key_Keypad7;
	GlobalkeyState.ScanCodeTable[0x048] = EKey::Key_Keypad8;
	GlobalkeyState.ScanCodeTable[0x049] = EKey::Key_Keypad9;
	GlobalkeyState.ScanCodeTable[0x04E] = EKey::Key_KeypadDecimal;
	GlobalkeyState.ScanCodeTable[0x053] = EKey::Key_KeypadDivide;
	GlobalkeyState.ScanCodeTable[0x135] = EKey::Key_KeypadMultiply;
	GlobalkeyState.ScanCodeTable[0x11C] = EKey::Key_KeypadSubtract;
	GlobalkeyState.ScanCodeTable[0x059] = EKey::Key_KeypadAdd;
	GlobalkeyState.ScanCodeTable[0x037] = EKey::Key_KeypadEnter;
	GlobalkeyState.ScanCodeTable[0x04A] = EKey::Key_KeypadEqual;

	GlobalkeyState.ScanCodeTable[0x02A] = EKey::Key_LeftShift;
	GlobalkeyState.ScanCodeTable[0x036] = EKey::Key_RightShift;
	GlobalkeyState.ScanCodeTable[0x01D] = EKey::Key_LeftControl;
	GlobalkeyState.ScanCodeTable[0x11D] = EKey::Key_RightControl;
	GlobalkeyState.ScanCodeTable[0x038] = EKey::Key_LeftAlt;
	GlobalkeyState.ScanCodeTable[0x138] = EKey::Key_RightAlt;
	GlobalkeyState.ScanCodeTable[0x15B] = EKey::Key_LeftSuper;
	GlobalkeyState.ScanCodeTable[0x15C] = EKey::Key_RightSuper;
	GlobalkeyState.ScanCodeTable[0x15D] = EKey::Key_Menu;
	GlobalkeyState.ScanCodeTable[0x039] = EKey::Key_Space;
	GlobalkeyState.ScanCodeTable[0x028] = EKey::Key_Apostrophe;
	GlobalkeyState.ScanCodeTable[0x033] = EKey::Key_Comma;
	GlobalkeyState.ScanCodeTable[0x00C] = EKey::Key_Minus;
	GlobalkeyState.ScanCodeTable[0x034] = EKey::Key_Period;
	GlobalkeyState.ScanCodeTable[0x035] = EKey::Key_Slash;
	GlobalkeyState.ScanCodeTable[0x027] = EKey::Key_Semicolon;
	GlobalkeyState.ScanCodeTable[0x00D] = EKey::Key_Equal;
	GlobalkeyState.ScanCodeTable[0x01A] = EKey::Key_LeftBracket;
	GlobalkeyState.ScanCodeTable[0x02B] = EKey::Key_Backslash;
	GlobalkeyState.ScanCodeTable[0x01B] = EKey::Key_RightBracket;
	GlobalkeyState.ScanCodeTable[0x029] = EKey::Key_GraveAccent;
	GlobalkeyState.ScanCodeTable[0x056] = EKey::Key_World2;
	GlobalkeyState.ScanCodeTable[0x001] = EKey::Key_Escape;
	GlobalkeyState.ScanCodeTable[0x01C] = EKey::Key_Enter;
	GlobalkeyState.ScanCodeTable[0x00F] = EKey::Key_Tab;
	GlobalkeyState.ScanCodeTable[0x00E] = EKey::Key_Backspace;
	GlobalkeyState.ScanCodeTable[0x152] = EKey::Key_Insert;
	GlobalkeyState.ScanCodeTable[0x153] = EKey::Key_Delete;
	GlobalkeyState.ScanCodeTable[0x14D] = EKey::Key_Right;
	GlobalkeyState.ScanCodeTable[0x14B] = EKey::Key_Left;
	GlobalkeyState.ScanCodeTable[0x150] = EKey::Key_Down;
	GlobalkeyState.ScanCodeTable[0x148] = EKey::Key_Up;
	GlobalkeyState.ScanCodeTable[0x149] = EKey::Key_PageUp;
	GlobalkeyState.ScanCodeTable[0x151] = EKey::Key_PageDown;
	GlobalkeyState.ScanCodeTable[0x147] = EKey::Key_Home;
	GlobalkeyState.ScanCodeTable[0x14F] = EKey::Key_End;
	GlobalkeyState.ScanCodeTable[0x03A] = EKey::Key_CapsLock;
	GlobalkeyState.ScanCodeTable[0x046] = EKey::Key_ScrollLock;
	GlobalkeyState.ScanCodeTable[0x145] = EKey::Key_NumLock;
	GlobalkeyState.ScanCodeTable[0x137] = EKey::Key_PrintScreen;
	GlobalkeyState.ScanCodeTable[0x146] = EKey::Key_Pause;

	for (UInt16 Index = 0; Index < 512; Index++)
	{
		if (GlobalkeyState.ScanCodeTable[Index] != EKey::Key_Unknown)
		{
			GlobalkeyState.KeyTable[GlobalkeyState.ScanCodeTable[Index]] = Index;
		}
	}

	GlobalkeyState.IsInitialized = true;
}

EKey Input::ConvertFromScanCode(UInt32 ScanCode)
{
	//[[unlikely]]
	if (!GlobalkeyState.IsInitialized)
	{
		InitializeKeyState();
	}

	return GlobalkeyState.ScanCodeTable[ScanCode];
}

UInt32 Input::ConvertToScanCode(EKey Key)
{
	//[[unlikely]]
	if (!GlobalkeyState.IsInitialized)
	{
		InitializeKeyState();
	}

	return GlobalkeyState.KeyTable[Key];
}

void Input::RegisterKeyUp(EKey Key)
{
	GlobalkeyState.KeyStates[Key] = false;
}

void Input::RegisterKeyDown(EKey Key)
{
	GlobalkeyState.KeyStates[Key] = true;
}

Bool Input::IsKeyUp(EKey Key)
{
	return !GlobalkeyState.KeyStates[Key];
}

Bool Input::IsKeyDown(EKey Key)
{
	return GlobalkeyState.KeyStates[Key];
}
