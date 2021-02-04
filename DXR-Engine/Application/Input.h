#pragma once
#include "InputCodes.h"

class Input
{
public:
    static EKey   ConvertFromScanCode(UInt32 ScanCode);
    static UInt32 ConvertToScanCode(EKey Key);

    static void RegisterKeyUp(EKey Key);
    static void RegisterKeyDown(EKey Key);

    static Bool IsKeyUp(EKey KeyCode);
    static Bool IsKeyDown(EKey KeyCode);

    static void ClearState();
};