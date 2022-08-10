#pragma once
#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericCursor

class COREAPPLICATION_API FGenericCursor 
    : public ICursor
{
public:
    FGenericCursor()
        : bIsVisible(true)
    { }

    virtual bool IsVisible() const override final { return bIsVisible; }

protected:
    bool bIsVisible;
};
