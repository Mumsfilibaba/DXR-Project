#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// State of which modifier keys are pressed

struct SModifierKeyState
{
public:

    SModifierKeyState() = default;

    FORCEINLINE SModifierKeyState(uint8 InModifierMask)
        : ModifierMask(InModifierMask)
    {
    }

    union
    {
        /* Flags */
        struct
        {
            uint8 bIsCtrlDown : 1;
            uint8 bIsAltDown : 1;
            uint8 bIsShiftDown : 1;
            uint8 bIsCapsLockDown : 1;
            uint8 bIsSuperKeyDown : 1;
            uint8 bIsNumPadDown : 1;
        };

        /* Mask */
        uint8 ModifierMask = 0;
    };
};