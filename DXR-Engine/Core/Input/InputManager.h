#pragma once
#include "Core/Application/Events.h"

class InputManager
{
public:
    bool Init();

    bool IsKeyUp( EKey KeyCode );
    bool IsKeyDown( EKey KeyCode );

    static InputManager& Get();

private:
    void OnKeyPressed( const KeyPressedEvent& Event );
    void OnKeyReleased( const KeyReleasedEvent& Event );

    void OnWindowFocusChanged( const WindowFocusChangedEvent& Event );

    TStaticArray<bool, EKey::Key_Count> KeyStates;
    TStaticArray<EKey, 512> KeyCodeFromScanCodeTable;
    TStaticArray<uint16, 512> KeyTable;
};