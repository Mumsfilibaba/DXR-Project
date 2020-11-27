#include "Input.h"

struct KeyTableState
{
	bool	KeyStates[EKey::KEY_COUNT];
	EKey	ScanCodeTable[512];
	UInt16	KeyTable[512];
	bool	IsInitialized = false;
};

static KeyTableState GlobalkeyState;

static void InitializeKeyState()
{
	Memory::Memzero(GlobalkeyState.KeyStates, sizeof(GlobalkeyState.KeyStates));
	Memory::Memzero(GlobalkeyState.ScanCodeTable, sizeof(GlobalkeyState.ScanCodeTable));
	Memory::Memzero(GlobalkeyState.KeyTable, sizeof(GlobalkeyState.KeyTable));

	GlobalkeyState.ScanCodeTable[0x00B] = EKey::KEY_0;
	GlobalkeyState.ScanCodeTable[0x002] = EKey::KEY_1;
	GlobalkeyState.ScanCodeTable[0x003] = EKey::KEY_2;
	GlobalkeyState.ScanCodeTable[0x004] = EKey::KEY_3;
	GlobalkeyState.ScanCodeTable[0x005] = EKey::KEY_4;
	GlobalkeyState.ScanCodeTable[0x006] = EKey::KEY_5;
	GlobalkeyState.ScanCodeTable[0x007] = EKey::KEY_6;
	GlobalkeyState.ScanCodeTable[0x008] = EKey::KEY_7;
	GlobalkeyState.ScanCodeTable[0x009] = EKey::KEY_8;
	GlobalkeyState.ScanCodeTable[0x00A] = EKey::KEY_9;

	GlobalkeyState.ScanCodeTable[0x01E] = EKey::KEY_A;
	GlobalkeyState.ScanCodeTable[0x030] = EKey::KEY_B;
	GlobalkeyState.ScanCodeTable[0x02E] = EKey::KEY_C;
	GlobalkeyState.ScanCodeTable[0x020] = EKey::KEY_D;
	GlobalkeyState.ScanCodeTable[0x012] = EKey::KEY_E;
	GlobalkeyState.ScanCodeTable[0x021] = EKey::KEY_F;
	GlobalkeyState.ScanCodeTable[0x022] = EKey::KEY_G;
	GlobalkeyState.ScanCodeTable[0x023] = EKey::KEY_H;
	GlobalkeyState.ScanCodeTable[0x017] = EKey::KEY_I;
	GlobalkeyState.ScanCodeTable[0x024] = EKey::KEY_J;
	GlobalkeyState.ScanCodeTable[0x025] = EKey::KEY_K;
	GlobalkeyState.ScanCodeTable[0x026] = EKey::KEY_L;
	GlobalkeyState.ScanCodeTable[0x032] = EKey::KEY_M;
	GlobalkeyState.ScanCodeTable[0x031] = EKey::KEY_N;
	GlobalkeyState.ScanCodeTable[0x018] = EKey::KEY_O;
	GlobalkeyState.ScanCodeTable[0x019] = EKey::KEY_P;
	GlobalkeyState.ScanCodeTable[0x010] = EKey::KEY_Q;
	GlobalkeyState.ScanCodeTable[0x013] = EKey::KEY_R;
	GlobalkeyState.ScanCodeTable[0x01F] = EKey::KEY_S;
	GlobalkeyState.ScanCodeTable[0x014] = EKey::KEY_T;
	GlobalkeyState.ScanCodeTable[0x016] = EKey::KEY_U;
	GlobalkeyState.ScanCodeTable[0x02F] = EKey::KEY_V;
	GlobalkeyState.ScanCodeTable[0x011] = EKey::KEY_W;
	GlobalkeyState.ScanCodeTable[0x02D] = EKey::KEY_X;
	GlobalkeyState.ScanCodeTable[0x015] = EKey::KEY_Y;
	GlobalkeyState.ScanCodeTable[0x02C] = EKey::KEY_Z;

	GlobalkeyState.ScanCodeTable[0x03B] = EKey::KEY_F1;
	GlobalkeyState.ScanCodeTable[0x03C] = EKey::KEY_F2;
	GlobalkeyState.ScanCodeTable[0x03D] = EKey::KEY_F3;
	GlobalkeyState.ScanCodeTable[0x03E] = EKey::KEY_F4;
	GlobalkeyState.ScanCodeTable[0x03F] = EKey::KEY_F5;
	GlobalkeyState.ScanCodeTable[0x040] = EKey::KEY_F6;
	GlobalkeyState.ScanCodeTable[0x041] = EKey::KEY_F7;
	GlobalkeyState.ScanCodeTable[0x042] = EKey::KEY_F8;
	GlobalkeyState.ScanCodeTable[0x043] = EKey::KEY_F9;
	GlobalkeyState.ScanCodeTable[0x044] = EKey::KEY_F10;
	GlobalkeyState.ScanCodeTable[0x057] = EKey::KEY_F11;
	GlobalkeyState.ScanCodeTable[0x058] = EKey::KEY_F12;
	GlobalkeyState.ScanCodeTable[0x064] = EKey::KEY_F13;
	GlobalkeyState.ScanCodeTable[0x065] = EKey::KEY_F14;
	GlobalkeyState.ScanCodeTable[0x066] = EKey::KEY_F15;
	GlobalkeyState.ScanCodeTable[0x067] = EKey::KEY_F16;
	GlobalkeyState.ScanCodeTable[0x068] = EKey::KEY_F17;
	GlobalkeyState.ScanCodeTable[0x069] = EKey::KEY_F18;
	GlobalkeyState.ScanCodeTable[0x06A] = EKey::KEY_F19;
	GlobalkeyState.ScanCodeTable[0x06B] = EKey::KEY_F20;
	GlobalkeyState.ScanCodeTable[0x06C] = EKey::KEY_F21;
	GlobalkeyState.ScanCodeTable[0x06D] = EKey::KEY_F22;
	GlobalkeyState.ScanCodeTable[0x06E] = EKey::KEY_F23;
	GlobalkeyState.ScanCodeTable[0x076] = EKey::KEY_F24;

	GlobalkeyState.ScanCodeTable[0x052] = EKey::KEY_KEYPAD_0;
	GlobalkeyState.ScanCodeTable[0x04F] = EKey::KEY_KEYPAD_1;
	GlobalkeyState.ScanCodeTable[0x050] = EKey::KEY_KEYPAD_2;
	GlobalkeyState.ScanCodeTable[0x051] = EKey::KEY_KEYPAD_3;
	GlobalkeyState.ScanCodeTable[0x04B] = EKey::KEY_KEYPAD_4;
	GlobalkeyState.ScanCodeTable[0x04C] = EKey::KEY_KEYPAD_5;
	GlobalkeyState.ScanCodeTable[0x04D] = EKey::KEY_KEYPAD_6;
	GlobalkeyState.ScanCodeTable[0x047] = EKey::KEY_KEYPAD_7;
	GlobalkeyState.ScanCodeTable[0x048] = EKey::KEY_KEYPAD_8;
	GlobalkeyState.ScanCodeTable[0x049] = EKey::KEY_KEYPAD_9;
	GlobalkeyState.ScanCodeTable[0x04E] = EKey::KEY_KEYPAD_DECIMAL;
	GlobalkeyState.ScanCodeTable[0x053] = EKey::KEY_KEYPAD_DIVIDE;
	GlobalkeyState.ScanCodeTable[0x135] = EKey::KEY_KEYPAD_MULTIPLY;
	GlobalkeyState.ScanCodeTable[0x11C] = EKey::KEY_KEYPAD_SUBTRACT;
	GlobalkeyState.ScanCodeTable[0x059] = EKey::KEY_KEYPAD_ADD;
	GlobalkeyState.ScanCodeTable[0x037] = EKey::KEY_KEYPAD_ENTER;
	GlobalkeyState.ScanCodeTable[0x04A] = EKey::KEY_KEYPAD_EQUAL;

	GlobalkeyState.ScanCodeTable[0x02A] = EKey::KEY_LEFT_SHIFT;
	GlobalkeyState.ScanCodeTable[0x036] = EKey::KEY_RIGHT_SHIFT;
	GlobalkeyState.ScanCodeTable[0x01D] = EKey::KEY_LEFT_CONTROL;
	GlobalkeyState.ScanCodeTable[0x11D] = EKey::KEY_RIGHT_CONTROL;
	GlobalkeyState.ScanCodeTable[0x038] = EKey::KEY_LEFT_ALT;
	GlobalkeyState.ScanCodeTable[0x138] = EKey::KEY_RIGHT_ALT;
	GlobalkeyState.ScanCodeTable[0x15B] = EKey::KEY_LEFT_SUPER;
	GlobalkeyState.ScanCodeTable[0x15C] = EKey::KEY_RIGHT_SUPER;
	GlobalkeyState.ScanCodeTable[0x15D] = EKey::KEY_MENU;
	GlobalkeyState.ScanCodeTable[0x039] = EKey::KEY_SPACE;
	GlobalkeyState.ScanCodeTable[0x028] = EKey::KEY_APOSTROPHE;
	GlobalkeyState.ScanCodeTable[0x033] = EKey::KEY_COMMA;
	GlobalkeyState.ScanCodeTable[0x00C] = EKey::KEY_MINUS;
	GlobalkeyState.ScanCodeTable[0x034] = EKey::KEY_PERIOD;
	GlobalkeyState.ScanCodeTable[0x035] = EKey::KEY_SLASH;
	GlobalkeyState.ScanCodeTable[0x027] = EKey::KEY_SEMICOLON;
	GlobalkeyState.ScanCodeTable[0x00D] = EKey::KEY_EQUAL;
	GlobalkeyState.ScanCodeTable[0x01A] = EKey::KEY_LEFT_BRACKET;
	GlobalkeyState.ScanCodeTable[0x02B] = EKey::KEY_BACKSLASH;
	GlobalkeyState.ScanCodeTable[0x01B] = EKey::KEY_RIGHT_BRACKET;
	GlobalkeyState.ScanCodeTable[0x029] = EKey::KEY_GRAVE_ACCENT;
	GlobalkeyState.ScanCodeTable[0x056] = EKey::KEY_WORLD_2;
	GlobalkeyState.ScanCodeTable[0x001] = EKey::KEY_ESCAPE;
	GlobalkeyState.ScanCodeTable[0x01C] = EKey::KEY_ENTER;
	GlobalkeyState.ScanCodeTable[0x00F] = EKey::KEY_TAB;
	GlobalkeyState.ScanCodeTable[0x00E] = EKey::KEY_BACKSPACE;
	GlobalkeyState.ScanCodeTable[0x152] = EKey::KEY_INSERT;
	GlobalkeyState.ScanCodeTable[0x153] = EKey::KEY_DELETE;
	GlobalkeyState.ScanCodeTable[0x14D] = EKey::KEY_RIGHT;
	GlobalkeyState.ScanCodeTable[0x14B] = EKey::KEY_LEFT;
	GlobalkeyState.ScanCodeTable[0x150] = EKey::KEY_DOWN;
	GlobalkeyState.ScanCodeTable[0x148] = EKey::KEY_UP;
	GlobalkeyState.ScanCodeTable[0x149] = EKey::KEY_PAGE_UP;
	GlobalkeyState.ScanCodeTable[0x151] = EKey::KEY_PAGE_DOWN;
	GlobalkeyState.ScanCodeTable[0x147] = EKey::KEY_HOME;
	GlobalkeyState.ScanCodeTable[0x14F] = EKey::KEY_END;
	GlobalkeyState.ScanCodeTable[0x03A] = EKey::KEY_CAPS_LOCK;
	GlobalkeyState.ScanCodeTable[0x046] = EKey::KEY_SCROLL_LOCK;
	GlobalkeyState.ScanCodeTable[0x145] = EKey::KEY_NUM_LOCK;
	GlobalkeyState.ScanCodeTable[0x137] = EKey::KEY_PRINT_SCREEN;
	GlobalkeyState.ScanCodeTable[0x146] = EKey::KEY_PAUSE;

	for (UInt16 Index = 0; Index < 512; Index++)
	{
		if (GlobalkeyState.ScanCodeTable[Index] != EKey::KEY_UNKNOWN)
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

bool Input::IsKeyUp(EKey Key)
{
	return !GlobalkeyState.KeyStates[Key];
}

bool Input::IsKeyDown(EKey Key)
{
	return GlobalkeyState.KeyStates[Key];
}
