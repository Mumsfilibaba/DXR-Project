#pragma once
#include "Core/Application/ICursor.h"

class CGenericCursor : public ICursor
{
public:

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const override final
    {
        return IsCursorVisible;
    }

    /* Check if the current button state is pressed */
    virtual bool IsButtonDown( EMouseButton Button ) const override final
    {
        return ButtonState[Button];
    }

    /* Check if the current button state is released */
    virtual bool IsButtonUp( EMouseButton Button ) const override final
    {
        return !ButtonState[Button];
    }

protected:

    CGenericCursor()
        : ICursor()
        , ButtonState()
        , IsCursorVisible( true )
    {
        Memory::Memzero( ButtonState.Data(), ButtonState.SizeInBytes() );
    }

    ~CGenericCursor() = default;

    /* The state of the mouse-buttons */
    TStaticArray<bool, EMouseButton::MouseButton_Count> ButtonState;

    /* Checks if the mouse is visible or not */
    bool IsCursorVisible;
};