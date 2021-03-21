#pragma once
#include "Events.h"

class InputManager
{
public:
    bool Init();

    bool IsKeyUp(EKey KeyCode);
    bool IsKeyDown(EKey KeyCode);

    EKey ConvertFromScanCode(uint32 ScanCode);
    uint32 ConvertToScanCode(EKey Key);

    static InputManager& Get();

private:
    void InitKeyTable();

    void OnKeyPressed(const KeyPressedEvent& Event);
    void OnKeyReleased(const KeyReleasedEvent& Event);

    void OnWindowFocusChanged(const WindowFocusChangedEvent& Event);

    TStaticArray<bool, EKey::Key_Count> KeyStates;
    TStaticArray<EKey, 512> ScanCodeTable;
    TStaticArray<uint16, 512> KeyTable;
};