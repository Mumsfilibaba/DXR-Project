#pragma once
#include "Core/Application/ICursorDevice.h"

class CGenericCursorDevice : public ICursorDevice
{
public:

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const override final
    {
        return IsCursorVisible;
    }

protected:

    FORCEINLINE CGenericCursorDevice()
        : ICursorDevice()
        , IsCursorVisible( true )
    {
    }

    ~CGenericCursorDevice() = default;

    /* Checks if the mouse is visible or not */
    bool IsCursorVisible;
};