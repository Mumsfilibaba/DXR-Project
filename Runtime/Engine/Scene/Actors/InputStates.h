#pragma once
#include "Application/Input/Keys.h"
#include "CoreApplication/Generic/InputCodes.h"

struct FKeyState
{
    FKeyState(FKey InKey)
        : Key(InKey)
        , bIsDown(0)
        , bPreviousState(0)
        , RepeatCount(0)
        , TimePressed(0.0f)
    {
    }

    /** @brief - The key for this key-state */
    const FKey Key;
    
    /** @brief - Indicates if the current state of the key is down */
    uint32 bIsDown : 1;
    
    /** @brief - Indicates if the last state of the key was down (1) or if it was up (0) */
    uint32 bPreviousState : 1;
    
    /** @brief - If this state is repeated due to the user holding down the key, the repeat-count is increased */
    uint32 RepeatCount : 30;

    /** @brief - Time pressed */
    float TimePressed = 0.0f;
};

struct FAnalogAxisState
{
    FAnalogAxisState(EAnalogSourceName::Type InSource)
        : Source(InSource)
        , Value(0.0f)
        , NumTicksSinceUpdate(0)
    {
    }

    /** @brief - The analog source that this came from */
    const EAnalogSourceName::Type Source;

    /** @brief - The current analog value */
    float Value;

    /** @brief - Amount of frames that has passed since the value was updated */
    int32 NumTicksSinceUpdate;
};
