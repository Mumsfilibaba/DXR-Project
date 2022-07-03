#pragma once
#include "InputCodes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Key-state - Holds information about how a key has been pressed

struct FKeyState
{
    FORCEINLINE FKeyState(EKey InKeyCode)
        : KeyCode(InKeyCode)
        , IsDown(0)
        , PreviousState(0)
        , RepeatCount(0)
        , TimePressed(0.0f)
    { }

    /** The key for this key-state */
    const EKey KeyCode;
    /** Indicates if the current state of the key is down */
    uint32 IsDown : 1;
    /** Indicates if the last state of the key was down (1) or if it was up (0) */
    uint32 PreviousState : 1;
    /** If this state is repeated due to the user holding down the key, the repeat-count is increased */
    uint32 RepeatCount : 30;
    /** Time pressed */
    float TimePressed = 0.0f;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MouseButtonState - Holds information about how a mouse-button has been pressed

struct FMouseButtonState
{
    FORCEINLINE FMouseButtonState(EMouseButton InButton)
        : Button(InButton)
        , IsDown(0)
        , PreviousState(0)
        , TimePressed(0.0f)
    { }

    /** The button associated with this button-state */
    const EMouseButton Button;
    /** Indicates if the current state of the button is down */
    uint32 IsDown : 1;
    /** Indicates if the last state of the button was down (1) or if it was up (0) */
    uint32 PreviousState : 1;
    /** Time pressed */
    float TimePressed = 0.0f;
};