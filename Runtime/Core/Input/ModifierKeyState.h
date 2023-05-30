#pragma once
#include "Core/Core.h"

struct FModifierKeyState
{
    FModifierKeyState() = default;

    FORCEINLINE FModifierKeyState(uint8 InModifierMask)
        : ModifierMask(InModifierMask)
    {
    }

    union
    {
         /** @brief - Flags */
        struct
        {
            uint8 bIsCtrlDown     : 1;
            uint8 bIsAltDown      : 1;
            uint8 bIsShiftDown    : 1;
            uint8 bIsCapsLockDown : 1;
            uint8 bIsSuperKeyDown : 1;
            uint8 bIsNumPadDown   : 1;
        };

         /** @brief - Mask */
        uint8 ModifierMask = 0;
    };
};