#pragma once
#include "CoreApplication/ICursor.h"

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
