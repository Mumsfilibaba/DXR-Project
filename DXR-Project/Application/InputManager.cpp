#include "InputManager.h"

InputManager::InputManager()
{
	for (Uint32 i = 0; i < EKey::KEY_COUNT; i++)
	{
		KeyStates[i] = false;
	}

	InitializeKeyTables();
}

InputManager::~InputManager()
{
}

void InputManager::InitializeKeyTables()
{
	ZERO_MEMORY(ScanCodeTable,	sizeof(ScanCodeTable));
	ZERO_MEMORY(KeyTable,		sizeof(KeyTable));

	ScanCodeTable[0x00B] = EKey::KEY_0;
	ScanCodeTable[0x002] = EKey::KEY_1;
	ScanCodeTable[0x003] = EKey::KEY_2;
	ScanCodeTable[0x004] = EKey::KEY_3;
	ScanCodeTable[0x005] = EKey::KEY_4;
	ScanCodeTable[0x006] = EKey::KEY_5;
	ScanCodeTable[0x007] = EKey::KEY_6;
	ScanCodeTable[0x008] = EKey::KEY_7;
	ScanCodeTable[0x009] = EKey::KEY_8;
	ScanCodeTable[0x00A] = EKey::KEY_9;

	ScanCodeTable[0x01E] = EKey::KEY_A;
	ScanCodeTable[0x030] = EKey::KEY_B;
	ScanCodeTable[0x02E] = EKey::KEY_C;
	ScanCodeTable[0x020] = EKey::KEY_D;
	ScanCodeTable[0x012] = EKey::KEY_E;
	ScanCodeTable[0x021] = EKey::KEY_F;
	ScanCodeTable[0x022] = EKey::KEY_G;
	ScanCodeTable[0x023] = EKey::KEY_H;
	ScanCodeTable[0x017] = EKey::KEY_I;
	ScanCodeTable[0x024] = EKey::KEY_J;
	ScanCodeTable[0x025] = EKey::KEY_K;
	ScanCodeTable[0x026] = EKey::KEY_L;
	ScanCodeTable[0x032] = EKey::KEY_M;
	ScanCodeTable[0x031] = EKey::KEY_N;
	ScanCodeTable[0x018] = EKey::KEY_O;
	ScanCodeTable[0x019] = EKey::KEY_P;
	ScanCodeTable[0x010] = EKey::KEY_Q;
	ScanCodeTable[0x013] = EKey::KEY_R;
	ScanCodeTable[0x01F] = EKey::KEY_S;
	ScanCodeTable[0x014] = EKey::KEY_T;
	ScanCodeTable[0x016] = EKey::KEY_U;
	ScanCodeTable[0x02F] = EKey::KEY_V;
	ScanCodeTable[0x011] = EKey::KEY_W;
	ScanCodeTable[0x02D] = EKey::KEY_X;
	ScanCodeTable[0x015] = EKey::KEY_Y;
	ScanCodeTable[0x02C] = EKey::KEY_Z;

	ScanCodeTable[0x03B] = EKey::KEY_F1;
	ScanCodeTable[0x03C] = EKey::KEY_F2;
	ScanCodeTable[0x03D] = EKey::KEY_F3;
	ScanCodeTable[0x03E] = EKey::KEY_F4;
	ScanCodeTable[0x03F] = EKey::KEY_F5;
	ScanCodeTable[0x040] = EKey::KEY_F6;
	ScanCodeTable[0x041] = EKey::KEY_F7;
	ScanCodeTable[0x042] = EKey::KEY_F8;
	ScanCodeTable[0x043] = EKey::KEY_F9;
	ScanCodeTable[0x044] = EKey::KEY_F10;
	ScanCodeTable[0x057] = EKey::KEY_F11;
	ScanCodeTable[0x058] = EKey::KEY_F12;
	ScanCodeTable[0x064] = EKey::KEY_F13;
	ScanCodeTable[0x065] = EKey::KEY_F14;
	ScanCodeTable[0x066] = EKey::KEY_F15;
	ScanCodeTable[0x067] = EKey::KEY_F16;
	ScanCodeTable[0x068] = EKey::KEY_F17;
	ScanCodeTable[0x069] = EKey::KEY_F18;
	ScanCodeTable[0x06A] = EKey::KEY_F19;
	ScanCodeTable[0x06B] = EKey::KEY_F20;
	ScanCodeTable[0x06C] = EKey::KEY_F21;
	ScanCodeTable[0x06D] = EKey::KEY_F22;
	ScanCodeTable[0x06E] = EKey::KEY_F23;
	ScanCodeTable[0x076] = EKey::KEY_F24;

	ScanCodeTable[0x052] = EKey::KEY_KEYPAD_0;
	ScanCodeTable[0x04F] = EKey::KEY_KEYPAD_1;
	ScanCodeTable[0x050] = EKey::KEY_KEYPAD_2;
	ScanCodeTable[0x051] = EKey::KEY_KEYPAD_3;
	ScanCodeTable[0x04B] = EKey::KEY_KEYPAD_4;
	ScanCodeTable[0x04C] = EKey::KEY_KEYPAD_5;
	ScanCodeTable[0x04D] = EKey::KEY_KEYPAD_6;
	ScanCodeTable[0x047] = EKey::KEY_KEYPAD_7;
	ScanCodeTable[0x048] = EKey::KEY_KEYPAD_8;
	ScanCodeTable[0x049] = EKey::KEY_KEYPAD_9;
	ScanCodeTable[0x04E] = EKey::KEY_KEYPAD_DECIMAL;
	ScanCodeTable[0x053] = EKey::KEY_KEYPAD_DIVIDE;
	ScanCodeTable[0x135] = EKey::KEY_KEYPAD_MULTIPLY;
	ScanCodeTable[0x11C] = EKey::KEY_KEYPAD_SUBTRACT;
	ScanCodeTable[0x059] = EKey::KEY_KEYPAD_ADD;
	ScanCodeTable[0x037] = EKey::KEY_KEYPAD_ENTER;
	ScanCodeTable[0x04A] = EKey::KEY_KEYPAD_EQUAL;

	ScanCodeTable[0x02A] = EKey::KEY_LEFT_SHIFT;
	ScanCodeTable[0x036] = EKey::KEY_RIGHT_SHIFT;
	ScanCodeTable[0x01D] = EKey::KEY_LEFT_CONTROL;
	ScanCodeTable[0x11D] = EKey::KEY_RIGHT_CONTROL;
	ScanCodeTable[0x038] = EKey::KEY_LEFT_ALT;
	ScanCodeTable[0x138] = EKey::KEY_RIGHT_ALT;
	ScanCodeTable[0x15B] = EKey::KEY_LEFT_SUPER;
	ScanCodeTable[0x15C] = EKey::KEY_RIGHT_SUPER;
	ScanCodeTable[0x15D] = EKey::KEY_MENU;
	ScanCodeTable[0x039] = EKey::KEY_SPACE;
	ScanCodeTable[0x028] = EKey::KEY_APOSTROPHE;
	ScanCodeTable[0x033] = EKey::KEY_COMMA;
	ScanCodeTable[0x00C] = EKey::KEY_MINUS;
	ScanCodeTable[0x034] = EKey::KEY_PERIOD;
	ScanCodeTable[0x035] = EKey::KEY_SLASH;
	ScanCodeTable[0x027] = EKey::KEY_SEMICOLON;
	ScanCodeTable[0x00D] = EKey::KEY_EQUAL;
	ScanCodeTable[0x01A] = EKey::KEY_LEFT_BRACKET;
	ScanCodeTable[0x02B] = EKey::KEY_BACKSLASH;
	ScanCodeTable[0x01B] = EKey::KEY_RIGHT_BRACKET;
	ScanCodeTable[0x029] = EKey::KEY_GRAVE_ACCENT;
	ScanCodeTable[0x056] = EKey::KEY_WORLD_2;
	ScanCodeTable[0x001] = EKey::KEY_ESCAPE;
	ScanCodeTable[0x01C] = EKey::KEY_ENTER;
	ScanCodeTable[0x00F] = EKey::KEY_TAB;
	ScanCodeTable[0x00E] = EKey::KEY_BACKSPACE;
	ScanCodeTable[0x152] = EKey::KEY_INSERT;
	ScanCodeTable[0x153] = EKey::KEY_DELETE;
	ScanCodeTable[0x14D] = EKey::KEY_RIGHT;
	ScanCodeTable[0x14B] = EKey::KEY_LEFT;
	ScanCodeTable[0x150] = EKey::KEY_DOWN;
	ScanCodeTable[0x148] = EKey::KEY_UP;
	ScanCodeTable[0x149] = EKey::KEY_PAGE_UP;
	ScanCodeTable[0x151] = EKey::KEY_PAGE_DOWN;
	ScanCodeTable[0x147] = EKey::KEY_HOME;
	ScanCodeTable[0x14F] = EKey::KEY_END;
	ScanCodeTable[0x03A] = EKey::KEY_CAPS_LOCK;
	ScanCodeTable[0x046] = EKey::KEY_SCROLL_LOCK;
	ScanCodeTable[0x145] = EKey::KEY_NUM_LOCK;
	ScanCodeTable[0x137] = EKey::KEY_PRINT_SCREEN;
	ScanCodeTable[0x146] = EKey::KEY_PAUSE;

	for (Uint32 i = 0; i < 512; i++)
	{
		if (ScanCodeTable[i] != EKey::KEY_UNKNOWN)
		{
			KeyTable[ScanCodeTable[i]] = i;
		}
	}
}

EKey InputManager::ConvertFromKeyCode(Uint32 KeyCode)
{
	return ScanCodeTable[KeyCode];
}

Uint32 InputManager::ConvertToKeyCode(EKey Key)
{
	return KeyTable[Key];
}

void InputManager::RegisterKeyUp(EKey KeyCode)
{
	KeyStates[KeyCode] = false;
}

void InputManager::RegisterKeyDown(EKey KeyCode)
{
	KeyStates[KeyCode] = true;
}

bool InputManager::IsKeyUp(EKey KeyCode) const
{
	return KeyStates[KeyCode] == false;
}

bool InputManager::IsKeyDown(EKey KeyCode) const
{
	return KeyStates[KeyCode] == true;
}

InputManager& InputManager::Get()
{
	static InputManager Instance;
	return Instance;
}
