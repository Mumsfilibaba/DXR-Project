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

protected:

    FORCEINLINE CGenericCursor()
        : ICursor()
        , IsCursorVisible( true )
    {
    }

    ~CGenericCursor() = default;

    /* Checks if the mouse is visible or not */
    bool IsCursorVisible;
};