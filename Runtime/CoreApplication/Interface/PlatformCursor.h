#pragma once
#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformCursor - Base Platform interface for cursor

class CPlatformCursor : public ICursor
{
public:

    /**
     * @brief: Retrieve the mouse visibility
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
