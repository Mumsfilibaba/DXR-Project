#pragma once 

#if defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericKeyboard.h"

class CMacKeyboard final : public CGenericKeyboard
{
    friend class CMacApplication;

private:

    CMacKeyboard()
        : CGenericKeyboard()
    {
    }

    ~CMacKeyboard() = default;

    // TODO: Maybe store all the keys in a bit-array? 
    FORCEINLINE void RegisterKeyState( EKey KeyCode, bool State )
    {
        KeyStates[KeyCode] = State;
    }
};

#endif
