#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Generic/GenericKeyboard.h"

class CWindowsKeyboard final : public CGenericKeyboard
{
    friend class CWindowsApplication;

private:

    FORCEINLINE CWindowsKeyboard()
        : CGenericKeyboard()
    {
    }

    ~CWindowsKeyboard() = default;

    // TODO: Maybe store all the keys in a bit-array? 
    FORCEINLINE void SetKeyState( EKey KeyCode, bool State )
    {
        KeyStates[KeyCode] = State;
    }
};

#endif