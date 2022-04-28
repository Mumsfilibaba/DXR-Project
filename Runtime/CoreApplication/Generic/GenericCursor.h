#pragma once
#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericCursor

class COREAPPLICATION_API CGenericCursor : public ICursor
{
protected:

    CGenericCursor()
        : bIsVisible(true)
    { }

    ~CGenericCursor() = default;

public:

    virtual bool IsVisible() const override final { return bIsVisible; }

protected:
    bool bIsVisible;
};