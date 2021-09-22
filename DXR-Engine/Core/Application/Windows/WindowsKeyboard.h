#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Generic/GenericKeyboard.h"

// TODO: The keyboard state probably has to tick to be the most accurate

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
    FORCEINLINE void RegisterKeyState( EKey KeyCode, bool State )
    {
        KeyStates[KeyCode] = State;
    }

    // When application loses focus, reset the state
    FORCEINLINE void ResetState()
    {
       Memory::Memzero( KeyStates.Data(), KeyStates.SizeInBytes() );
    }
};

#endif