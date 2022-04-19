#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SModifierKeyState

struct SModifierKeyState
{
    SModifierKeyState()
        : ModifierMask(0)
    { }

    SModifierKeyState(uint8 InModifierMask)
        : ModifierMask(InModifierMask)
    { }

    bool operator==(SModifierKeyState RHS) const
    {
        return (ModifierMask == RHS.ModifierMask);
    }

    union
    {
        /* Flags */
        struct
        {
            uint8 bIsCtrlDown     : 1;
            uint8 bIsAltDown      : 1;
            uint8 bIsShiftDown    : 1;
            uint8 bIsCapsLockDown : 1;
            uint8 bIsSuperKeyDown : 1;
            uint8 bIsNumPadDown   : 1;
        };

        /* Mask */
        uint8 ModifierMask = 0;
    };
};