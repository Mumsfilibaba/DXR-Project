#pragma once
#include "Core.h"

/* Struct containing the state of modifierkeys */
struct SModifierKeyState
{
public:
    
    SModifierKeyState() = default;

    FORCEINLINE SModifierKeyState( uint8 InModifierMask )
        : ModifierMask( InModifierMask )
    {
    }

    union
    {
        /* Flags */
        struct
        {
            uint8 IsCtrlDown : 1;
            uint8 IsAltDown : 1;
            uint8 IsShiftDown : 1;
            uint8 IsCapsLockDown : 1;
            uint8 IsSuperKeyDown : 1;
            uint8 IsNumPadDown : 1;
        };

        /* Mask */
        uint8 ModifierMask = 0;
    };
};