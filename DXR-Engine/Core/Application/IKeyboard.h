#pragma once
#include "Core/Input/InputCodes.h"

class IKeyboard
{
public:

    /* Check if the current key-state is pressed */
    virtual bool IsKeyDown( EKey KeyCode ) const = 0;

    /* Check if the current key-state is released */
    virtual bool IsKeyUp( EKey KeyCode ) const = 0;

protected:
    ~IKeyboard() = default;
};