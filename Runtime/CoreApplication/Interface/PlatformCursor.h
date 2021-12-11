#pragma once
#include "CoreApplication/ICursor.h"

class CPlatformCursor : public ICursor
{
public:

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const override final
    {
        return bIsVisible;
    }

protected:

    FORCEINLINE CPlatformCursor()
        : ICursor()
        , bIsVisible( true )
    {
    }

    ~CPlatformCursor() = default;

    /* Checks if the mouse is visible or not */
    bool bIsVisible;
};