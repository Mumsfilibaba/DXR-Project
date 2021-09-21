#pragma once
#include "Core/Application/IKeyboard.h"
#include "Core/Containers/StaticArray.h"

/* Holds the current key-state of the keyboard */
class CGenericKeyboard : public IKeyboard
{
public:

    /* Check if the current key-state is pressed */
    virtual bool IsKeyDown( EKey KeyCode ) const override
    {
        return (KeyStates[KeyCode] == true);
    }

    /* Check if the current key-state is released */
    virtual bool IsKeyUp( EKey KeyCode ) const override
    {
        return (KeyStates[KeyCode] == false);
    }

protected:

    FORCEINLINE CGenericKeyboard()
        : KeyStates()
    {
        Memory::Memzero( KeyStates.Data(), KeyStates.SizeInBytes() );
    }

    ~CGenericKeyboard() = default;

    /* The state of each key */
    TStaticArray<bool, EKey::Key_Count> KeyStates;
};