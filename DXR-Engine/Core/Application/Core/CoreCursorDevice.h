#pragma once
#include "Core/Application/ICursorDevice.h"

class CCoreCursorDevice : public ICursorDevice
{
public:

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const override final
    {
        return IsCursorVisible;
    }

protected:

    FORCEINLINE CCoreCursorDevice()
        : ICursorDevice()
        , IsCursorVisible( true )
    {
    }

    ~CCoreCursorDevice() = default;

    /* Checks if the mouse is visible or not */
    bool IsCursorVisible;
};