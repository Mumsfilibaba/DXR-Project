#pragma once
#include "Core/Application/ICursor.h"

class CCoreCursor : public ICursor
{
public:

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const override final
    {
        return IsCursorVisible;
    }

protected:

    FORCEINLINE CCoreCursor()
        : ICursor()
        , IsCursorVisible( true )
    {
    }

    ~CCoreCursor() = default;

    /* Checks if the mouse is visible or not */
    bool IsCursorVisible;
};