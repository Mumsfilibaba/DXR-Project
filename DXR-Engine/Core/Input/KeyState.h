#pragma once
#include "InputCodes.h"

// TODO: Good idea but needs to be fleshed out more
struct SKeyState
{
    FORCEINLINE SKeyState( EKey InKeyCode, bool InIsDown, bool InPreviousState, uint8 InRepeatCount )
        : KeyCode( InKeyCode )
        , IsDown( static_cast<uint8>(InIsDown) )
        , PreviousState( static_cast<uint8>(InPreviousState) )
        , RepeatCount( InRepeatCount )
    {
    }

    /* The key for this keystate */
    EKey KeyCode;
    
    /* Indicates if the current state of the key is down */
    uint8 IsDown : 1;

    /* Indicates if the last state of the key was down (1) or if it was up (0) */
    uint8 PreviousState : 1;

    /* If this state is repeated due to the user holding down the button, the repeatcount is increased */
    uint8 RepeatCount : 6;
};
