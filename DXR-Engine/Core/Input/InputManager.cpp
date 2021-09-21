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
    return KeyCodeFromScanCodeTable[ScanCode];
}

uint32 InputManager::ConvertToScanCode( EKey KeyCode )
{
    return KeyTable[KeyCode];
}

void InputManager::InitKeyTable()
{

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
