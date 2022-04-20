#pragma once
#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformCursor - Base Platform interface for cursor

class CPlatformCursor : public ICursor
{
protected:

    CPlatformCursor()  = default;
    ~CPlatformCursor() = default;

public:

    /** @return: Returns true if the cursor is visible */
    virtual bool IsVisible() const override final
    {
        return bIsVisible;
    }

protected:
    bool bIsVisible = true;
};
