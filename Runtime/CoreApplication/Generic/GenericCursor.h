#pragma once
#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericCursor

class CGenericCursor : public ICursor
{
protected:

    CGenericCursor() = default;
    ~CGenericCursor() = default;

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
    bool bIsVisible = true;
};