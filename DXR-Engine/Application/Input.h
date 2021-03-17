#pragma once
#include "InputCodes.h"

class Input
{
public:
    static EKey   ConvertFromScanCode(uint32 ScanCode);
    static uint32 ConvertToScanCode(EKey Key);

    static void RegisterKeyUp(EKey Key);
    static void RegisterKeyDown(EKey Key);

    static bool IsKeyUp(EKey KeyCode);
    static bool IsKeyDown(EKey KeyCode);

    static void ClearState();
};