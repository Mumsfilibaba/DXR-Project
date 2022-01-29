#pragma once
#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Base Platform interface for cursor

class CPlatformCursor : public ICursor
{
public:

    /**
     * Retrieve the mouse visibility
     * 
     * @return: Returns true if the cursor is visible
     */
    virtual bool IsVisible() const override final
    {
        return bIsVisible;
    }

protected:

    CPlatformCursor() = default;
    ~CPlatformCursor() = default;

    bool bIsVisible = true;
};