#include "InputManager.h"

#include "Core/Engine/Engine.h"

InputManager GInputManager;

bool InputManager::Init()
{
    InitKeyTable();

	GEngine->OnKeyPressedEvent.AddRaw( this, &InputManager::OnKeyPressed );
	GEngine->OnKeyReleasedEvent.AddRaw( this, &InputManager::OnKeyReleased );
	GEngine->OnWindowFocusChangedEvent.AddRaw( this, &InputManager::OnWindowFocusChanged );

    return true;
}

bool InputManager::IsKeyUp( EKey KeyCode )
{
    return !KeyStates[KeyCode];
}

bool InputManager::IsKeyDown( EKey KeyCode )
{
    return KeyStates[KeyCode];
}

InputManager& InputManager::Get()
{
    return GInputManager;
}

EKey InputManager::ConvertFromScanCode( uint32 ScanCode )
{
    return ScanCodeTable[ScanCode];
}

uint32 InputManager::ConvertToScanCode( EKey KeyCode )
{
    return KeyTable[KeyCode];
}

void InputManager::InitKeyTable()
{
    Memory::Memzero( KeyStates.Data(), KeyStates.SizeInBytes() );
    Memory::Memzero( ScanCodeTable.Data(), ScanCodeTable.SizeInBytes() );
    Memory::Memzero( KeyTable.Data(), KeyTable.SizeInBytes() );

    ScanCodeTable[0x00B] = EKey::Key_0;
    ScanCodeTable[0x002] = EKey::Key_1;
    ScanCodeTable[0x003] = EKey::Key_2;
    ScanCodeTable[0x004] = EKey::Key_3;
    ScanCodeTable[0x005] = EKey::Key_4;
    ScanCodeTable[0x006] = EKey::Key_5;
    ScanCodeTable[0x007] = EKey::Key_6;
    ScanCodeTable[0x008] = EKey::Key_7;
    ScanCodeTable[0x009] = EKey::Key_8;
    ScanCodeTable[0x00A] = EKey::Key_9;

    ScanCodeTable[0x01E] = EKey::Key_A;
    ScanCodeTable[0x030] = EKey::Key_B;
    ScanCodeTable[0x02E] = EKey::Key_C;
    ScanCodeTable[0x020] = EKey::Key_D;
    ScanCodeTable[0x012] = EKey::Key_E;
    ScanCodeTable[0x021] = EKey::Key_F;
    ScanCodeTable[0x022] = EKey::Key_G;
    ScanCodeTable[0x023] = EKey::Key_H;
    ScanCodeTable[0x017] = EKey::Key_I;
    ScanCodeTable[0x024] = EKey::Key_J;
    ScanCodeTable[0x025] = EKey::Key_K;
    ScanCodeTable[0x026] = EKey::Key_L;
    ScanCodeTable[0x032] = EKey::Key_M;
    ScanCodeTable[0x031] = EKey::Key_N;
    ScanCodeTable[0x018] = EKey::Key_O;
    ScanCodeTable[0x019] = EKey::Key_P;
    ScanCodeTable[0x010] = EKey::Key_Q;
    ScanCodeTable[0x013] = EKey::Key_R;
    ScanCodeTable[0x01F] = EKey::Key_S;
    ScanCodeTable[0x014] = EKey::Key_T;
    ScanCodeTable[0x016] = EKey::Key_U;
    ScanCodeTable[0x02F] = EKey::Key_V;
    ScanCodeTable[0x011] = EKey::Key_W;
    ScanCodeTable[0x02D] = EKey::Key_X;
    ScanCodeTable[0x015] = EKey::Key_Y;
    ScanCodeTable[0x02C] = EKey::Key_Z;

    ScanCodeTable[0x03B] = EKey::Key_F1;
    ScanCodeTable[0x03C] = EKey::Key_F2;
    ScanCodeTable[0x03D] = EKey::Key_F3;
    ScanCodeTable[0x03E] = EKey::Key_F4;
    ScanCodeTable[0x03F] = EKey::Key_F5;
    ScanCodeTable[0x040] = EKey::Key_F6;
    ScanCodeTable[0x041] = EKey::Key_F7;
    ScanCodeTable[0x042] = EKey::Key_F8;
    ScanCodeTable[0x043] = EKey::Key_F9;
    ScanCodeTable[0x044] = EKey::Key_F10;
    ScanCodeTable[0x057] = EKey::Key_F11;
    ScanCodeTable[0x058] = EKey::Key_F12;
    ScanCodeTable[0x064] = EKey::Key_F13;
    ScanCodeTable[0x065] = EKey::Key_F14;
    ScanCodeTable[0x066] = EKey::Key_F15;
    ScanCodeTable[0x067] = EKey::Key_F16;
    ScanCodeTable[0x068] = EKey::Key_F17;
    ScanCodeTable[0x069] = EKey::Key_F18;
    ScanCodeTable[0x06A] = EKey::Key_F19;
    ScanCodeTable[0x06B] = EKey::Key_F20;
    ScanCodeTable[0x06C] = EKey::Key_F21;
    ScanCodeTable[0x06D] = EKey::Key_F22;
    ScanCodeTable[0x06E] = EKey::Key_F23;
    ScanCodeTable[0x076] = EKey::Key_F24;

    ScanCodeTable[0x052] = EKey::Key_Keypad0;
    ScanCodeTable[0x04F] = EKey::Key_Keypad1;
    ScanCodeTable[0x050] = EKey::Key_Keypad2;
    ScanCodeTable[0x051] = EKey::Key_Keypad3;
    ScanCodeTable[0x04B] = EKey::Key_Keypad4;
    ScanCodeTable[0x04C] = EKey::Key_Keypad5;
    ScanCodeTable[0x04D] = EKey::Key_Keypad6;
    ScanCodeTable[0x047] = EKey::Key_Keypad7;
    ScanCodeTable[0x048] = EKey::Key_Keypad8;
    ScanCodeTable[0x049] = EKey::Key_Keypad9;
    ScanCodeTable[0x04E] = EKey::Key_KeypadAdd;
    ScanCodeTable[0x053] = EKey::Key_KeypadDecimal;
    ScanCodeTable[0x135] = EKey::Key_KeypadDivide;
    ScanCodeTable[0x11C] = EKey::Key_KeypadEnter;
    ScanCodeTable[0x059] = EKey::Key_KeypadEqual;
    ScanCodeTable[0x037] = EKey::Key_KeypadMultiply;
    ScanCodeTable[0x04A] = EKey::Key_KeypadSubtract;

    ScanCodeTable[0x02A] = EKey::Key_LeftShift;
    ScanCodeTable[0x036] = EKey::Key_RightShift;
    ScanCodeTable[0x01D] = EKey::Key_LeftControl;
    ScanCodeTable[0x11D] = EKey::Key_RightControl;
    ScanCodeTable[0x038] = EKey::Key_LeftAlt;
    ScanCodeTable[0x138] = EKey::Key_RightAlt;
    ScanCodeTable[0x15B] = EKey::Key_LeftSuper;
    ScanCodeTable[0x15C] = EKey::Key_RightSuper;
    ScanCodeTable[0x15D] = EKey::Key_Menu;
    ScanCodeTable[0x039] = EKey::Key_Space;
    ScanCodeTable[0x028] = EKey::Key_Apostrophe;
    ScanCodeTable[0x033] = EKey::Key_Comma;
    ScanCodeTable[0x00C] = EKey::Key_Minus;
    ScanCodeTable[0x034] = EKey::Key_Period;
    ScanCodeTable[0x035] = EKey::Key_Slash;
    ScanCodeTable[0x027] = EKey::Key_Semicolon;
    ScanCodeTable[0x00D] = EKey::Key_Equal;
    ScanCodeTable[0x01A] = EKey::Key_LeftBracket;
    ScanCodeTable[0x02B] = EKey::Key_Backslash;
    ScanCodeTable[0x01B] = EKey::Key_RightBracket;
    ScanCodeTable[0x029] = EKey::Key_GraveAccent;
    ScanCodeTable[0x056] = EKey::Key_World2;
    ScanCodeTable[0x001] = EKey::Key_Escape;
    ScanCodeTable[0x01C] = EKey::Key_Enter;
    ScanCodeTable[0x00F] = EKey::Key_Tab;
    ScanCodeTable[0x00E] = EKey::Key_Backspace;
    ScanCodeTable[0x152] = EKey::Key_Insert;
    ScanCodeTable[0x153] = EKey::Key_Delete;
    ScanCodeTable[0x14D] = EKey::Key_Right;
    ScanCodeTable[0x14B] = EKey::Key_Left;
    ScanCodeTable[0x150] = EKey::Key_Down;
    ScanCodeTable[0x148] = EKey::Key_Up;
    ScanCodeTable[0x149] = EKey::Key_PageUp;
    ScanCodeTable[0x151] = EKey::Key_PageDown;
    ScanCodeTable[0x147] = EKey::Key_Home;
    ScanCodeTable[0x14F] = EKey::Key_End;
    ScanCodeTable[0x03A] = EKey::Key_CapsLock;
    ScanCodeTable[0x046] = EKey::Key_ScrollLock;
    ScanCodeTable[0x145] = EKey::Key_NumLock;
    ScanCodeTable[0x137] = EKey::Key_PrintScreen;
    ScanCodeTable[0x146] = EKey::Key_Pause;

    for ( uint16 Index = 0; Index < 512; Index++ )
    {
        if ( ScanCodeTable[Index] != EKey::Key_Unknown )
        {
            KeyTable[ScanCodeTable[Index]] = Index;
        }
    }
}

void InputManager::OnKeyPressed( const KeyPressedEvent& Event )
{
    // TODO: Maybe a better solution that this?
    ImGuiIO& IO = ImGui::GetIO();
    if ( !IO.WantCaptureKeyboard )
    {
        KeyStates[Event.Key] = true;
    }
}

void InputManager::OnKeyReleased( const KeyReleasedEvent& Event )
{
    KeyStates[Event.Key] = false;
}

void InputManager::OnWindowFocusChanged( const WindowFocusChangedEvent& Event )
{
    if ( !Event.HasFocus )
    {
        KeyStates.Fill( false );
    }
}
