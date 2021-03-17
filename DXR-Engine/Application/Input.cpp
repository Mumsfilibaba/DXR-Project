#include "Input.h"

struct KeyTableState
{
    TStaticArray<bool, EKey::Key_Count> KeyStates;
    TStaticArray<EKey, 512>   ScanCodeTable;
    TStaticArray<uint16, 512> KeyTable;
    bool IsInitialized = false;
};

static KeyTableState GlobalKeyState;

static void InitializeKeyState()
{
    Memory::Memzero(GlobalKeyState.KeyStates.Data(), GlobalKeyState.KeyStates.SizeInBytes());
    Memory::Memzero(GlobalKeyState.ScanCodeTable.Data(), GlobalKeyState.ScanCodeTable.SizeInBytes());
    Memory::Memzero(GlobalKeyState.KeyTable.Data(), GlobalKeyState.KeyTable.SizeInBytes());

    GlobalKeyState.ScanCodeTable[0x00B] = EKey::Key_0;
    GlobalKeyState.ScanCodeTable[0x002] = EKey::Key_1;
    GlobalKeyState.ScanCodeTable[0x003] = EKey::Key_2;
    GlobalKeyState.ScanCodeTable[0x004] = EKey::Key_3;
    GlobalKeyState.ScanCodeTable[0x005] = EKey::Key_4;
    GlobalKeyState.ScanCodeTable[0x006] = EKey::Key_5;
    GlobalKeyState.ScanCodeTable[0x007] = EKey::Key_6;
    GlobalKeyState.ScanCodeTable[0x008] = EKey::Key_7;
    GlobalKeyState.ScanCodeTable[0x009] = EKey::Key_8;
    GlobalKeyState.ScanCodeTable[0x00A] = EKey::Key_9;

    GlobalKeyState.ScanCodeTable[0x01E] = EKey::Key_A;
    GlobalKeyState.ScanCodeTable[0x030] = EKey::Key_B;
    GlobalKeyState.ScanCodeTable[0x02E] = EKey::Key_C;
    GlobalKeyState.ScanCodeTable[0x020] = EKey::Key_D;
    GlobalKeyState.ScanCodeTable[0x012] = EKey::Key_E;
    GlobalKeyState.ScanCodeTable[0x021] = EKey::Key_F;
    GlobalKeyState.ScanCodeTable[0x022] = EKey::Key_G;
    GlobalKeyState.ScanCodeTable[0x023] = EKey::Key_H;
    GlobalKeyState.ScanCodeTable[0x017] = EKey::Key_I;
    GlobalKeyState.ScanCodeTable[0x024] = EKey::Key_J;
    GlobalKeyState.ScanCodeTable[0x025] = EKey::Key_K;
    GlobalKeyState.ScanCodeTable[0x026] = EKey::Key_L;
    GlobalKeyState.ScanCodeTable[0x032] = EKey::Key_M;
    GlobalKeyState.ScanCodeTable[0x031] = EKey::Key_N;
    GlobalKeyState.ScanCodeTable[0x018] = EKey::Key_O;
    GlobalKeyState.ScanCodeTable[0x019] = EKey::Key_P;
    GlobalKeyState.ScanCodeTable[0x010] = EKey::Key_Q;
    GlobalKeyState.ScanCodeTable[0x013] = EKey::Key_R;
    GlobalKeyState.ScanCodeTable[0x01F] = EKey::Key_S;
    GlobalKeyState.ScanCodeTable[0x014] = EKey::Key_T;
    GlobalKeyState.ScanCodeTable[0x016] = EKey::Key_U;
    GlobalKeyState.ScanCodeTable[0x02F] = EKey::Key_V;
    GlobalKeyState.ScanCodeTable[0x011] = EKey::Key_W;
    GlobalKeyState.ScanCodeTable[0x02D] = EKey::Key_X;
    GlobalKeyState.ScanCodeTable[0x015] = EKey::Key_Y;
    GlobalKeyState.ScanCodeTable[0x02C] = EKey::Key_Z;

    GlobalKeyState.ScanCodeTable[0x03B] = EKey::Key_F1;
    GlobalKeyState.ScanCodeTable[0x03C] = EKey::Key_F2;
    GlobalKeyState.ScanCodeTable[0x03D] = EKey::Key_F3;
    GlobalKeyState.ScanCodeTable[0x03E] = EKey::Key_F4;
    GlobalKeyState.ScanCodeTable[0x03F] = EKey::Key_F5;
    GlobalKeyState.ScanCodeTable[0x040] = EKey::Key_F6;
    GlobalKeyState.ScanCodeTable[0x041] = EKey::Key_F7;
    GlobalKeyState.ScanCodeTable[0x042] = EKey::Key_F8;
    GlobalKeyState.ScanCodeTable[0x043] = EKey::Key_F9;
    GlobalKeyState.ScanCodeTable[0x044] = EKey::Key_F10;
    GlobalKeyState.ScanCodeTable[0x057] = EKey::Key_F11;
    GlobalKeyState.ScanCodeTable[0x058] = EKey::Key_F12;
    GlobalKeyState.ScanCodeTable[0x064] = EKey::Key_F13;
    GlobalKeyState.ScanCodeTable[0x065] = EKey::Key_F14;
    GlobalKeyState.ScanCodeTable[0x066] = EKey::Key_F15;
    GlobalKeyState.ScanCodeTable[0x067] = EKey::Key_F16;
    GlobalKeyState.ScanCodeTable[0x068] = EKey::Key_F17;
    GlobalKeyState.ScanCodeTable[0x069] = EKey::Key_F18;
    GlobalKeyState.ScanCodeTable[0x06A] = EKey::Key_F19;
    GlobalKeyState.ScanCodeTable[0x06B] = EKey::Key_F20;
    GlobalKeyState.ScanCodeTable[0x06C] = EKey::Key_F21;
    GlobalKeyState.ScanCodeTable[0x06D] = EKey::Key_F22;
    GlobalKeyState.ScanCodeTable[0x06E] = EKey::Key_F23;
    GlobalKeyState.ScanCodeTable[0x076] = EKey::Key_F24;

    GlobalKeyState.ScanCodeTable[0x052] = EKey::Key_Keypad0;
    GlobalKeyState.ScanCodeTable[0x04F] = EKey::Key_Keypad1;
    GlobalKeyState.ScanCodeTable[0x050] = EKey::Key_Keypad2;
    GlobalKeyState.ScanCodeTable[0x051] = EKey::Key_Keypad3;
    GlobalKeyState.ScanCodeTable[0x04B] = EKey::Key_Keypad4;
    GlobalKeyState.ScanCodeTable[0x04C] = EKey::Key_Keypad5;
    GlobalKeyState.ScanCodeTable[0x04D] = EKey::Key_Keypad6;
    GlobalKeyState.ScanCodeTable[0x047] = EKey::Key_Keypad7;
    GlobalKeyState.ScanCodeTable[0x048] = EKey::Key_Keypad8;
    GlobalKeyState.ScanCodeTable[0x049] = EKey::Key_Keypad9;
    GlobalKeyState.ScanCodeTable[0x04E] = EKey::Key_KeypadAdd;
    GlobalKeyState.ScanCodeTable[0x053] = EKey::Key_KeypadDecimal;
    GlobalKeyState.ScanCodeTable[0x135] = EKey::Key_KeypadDivide;
    GlobalKeyState.ScanCodeTable[0x11C] = EKey::Key_KeypadEnter;
    GlobalKeyState.ScanCodeTable[0x059] = EKey::Key_KeypadEqual;
    GlobalKeyState.ScanCodeTable[0x037] = EKey::Key_KeypadMultiply;
    GlobalKeyState.ScanCodeTable[0x04A] = EKey::Key_KeypadSubtract;

    GlobalKeyState.ScanCodeTable[0x02A] = EKey::Key_LeftShift;
    GlobalKeyState.ScanCodeTable[0x036] = EKey::Key_RightShift;
    GlobalKeyState.ScanCodeTable[0x01D] = EKey::Key_LeftControl;
    GlobalKeyState.ScanCodeTable[0x11D] = EKey::Key_RightControl;
    GlobalKeyState.ScanCodeTable[0x038] = EKey::Key_LeftAlt;
    GlobalKeyState.ScanCodeTable[0x138] = EKey::Key_RightAlt;
    GlobalKeyState.ScanCodeTable[0x15B] = EKey::Key_LeftSuper;
    GlobalKeyState.ScanCodeTable[0x15C] = EKey::Key_RightSuper;
    GlobalKeyState.ScanCodeTable[0x15D] = EKey::Key_Menu;
    GlobalKeyState.ScanCodeTable[0x039] = EKey::Key_Space;
    GlobalKeyState.ScanCodeTable[0x028] = EKey::Key_Apostrophe;
    GlobalKeyState.ScanCodeTable[0x033] = EKey::Key_Comma;
    GlobalKeyState.ScanCodeTable[0x00C] = EKey::Key_Minus;
    GlobalKeyState.ScanCodeTable[0x034] = EKey::Key_Period;
    GlobalKeyState.ScanCodeTable[0x035] = EKey::Key_Slash;
    GlobalKeyState.ScanCodeTable[0x027] = EKey::Key_Semicolon;
    GlobalKeyState.ScanCodeTable[0x00D] = EKey::Key_Equal;
    GlobalKeyState.ScanCodeTable[0x01A] = EKey::Key_LeftBracket;
    GlobalKeyState.ScanCodeTable[0x02B] = EKey::Key_Backslash;
    GlobalKeyState.ScanCodeTable[0x01B] = EKey::Key_RightBracket;
    GlobalKeyState.ScanCodeTable[0x029] = EKey::Key_GraveAccent;
    GlobalKeyState.ScanCodeTable[0x056] = EKey::Key_World2;
    GlobalKeyState.ScanCodeTable[0x001] = EKey::Key_Escape;
    GlobalKeyState.ScanCodeTable[0x01C] = EKey::Key_Enter;
    GlobalKeyState.ScanCodeTable[0x00F] = EKey::Key_Tab;
    GlobalKeyState.ScanCodeTable[0x00E] = EKey::Key_Backspace;
    GlobalKeyState.ScanCodeTable[0x152] = EKey::Key_Insert;
    GlobalKeyState.ScanCodeTable[0x153] = EKey::Key_Delete;
    GlobalKeyState.ScanCodeTable[0x14D] = EKey::Key_Right;
    GlobalKeyState.ScanCodeTable[0x14B] = EKey::Key_Left;
    GlobalKeyState.ScanCodeTable[0x150] = EKey::Key_Down;
    GlobalKeyState.ScanCodeTable[0x148] = EKey::Key_Up;
    GlobalKeyState.ScanCodeTable[0x149] = EKey::Key_PageUp;
    GlobalKeyState.ScanCodeTable[0x151] = EKey::Key_PageDown;
    GlobalKeyState.ScanCodeTable[0x147] = EKey::Key_Home;
    GlobalKeyState.ScanCodeTable[0x14F] = EKey::Key_End;
    GlobalKeyState.ScanCodeTable[0x03A] = EKey::Key_CapsLock;
    GlobalKeyState.ScanCodeTable[0x046] = EKey::Key_ScrollLock;
    GlobalKeyState.ScanCodeTable[0x145] = EKey::Key_NumLock;
    GlobalKeyState.ScanCodeTable[0x137] = EKey::Key_PrintScreen;
    GlobalKeyState.ScanCodeTable[0x146] = EKey::Key_Pause;

    for (uint16 Index = 0; Index < 512; Index++)
    {
        if (GlobalKeyState.ScanCodeTable[Index] != EKey::Key_Unknown)
        {
            GlobalKeyState.KeyTable[GlobalKeyState.ScanCodeTable[Index]] = Index;
        }
    }

    GlobalKeyState.IsInitialized = true;
}

EKey Input::ConvertFromScanCode(uint32 ScanCode)
{
    //[[unlikely]]
    if (!GlobalKeyState.IsInitialized)
    {
        InitializeKeyState();
    }

    return GlobalKeyState.ScanCodeTable[ScanCode];
}

uint32 Input::ConvertToScanCode(EKey Key)
{
    //[[unlikely]]
    if (!GlobalKeyState.IsInitialized)
    {
        InitializeKeyState();
    }

    return GlobalKeyState.KeyTable[Key];
}

void Input::RegisterKeyUp(EKey Key)
{
    GlobalKeyState.KeyStates[Key] = false;
}

void Input::RegisterKeyDown(EKey Key)
{
    GlobalKeyState.KeyStates[Key] = true;
}

bool Input::IsKeyUp(EKey Key)
{
    return !GlobalKeyState.KeyStates[Key];
}

bool Input::IsKeyDown(EKey Key)
{
    return GlobalKeyState.KeyStates[Key];
}

void Input::ClearState()
{
    GlobalKeyState.KeyStates.Fill(false);
}
