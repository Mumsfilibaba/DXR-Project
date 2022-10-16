#pragma once
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/Generic/GenericCursor.h"

class COREAPPLICATION_API FMacCursor final 
    : public FGenericCursor
{
public:
    FMacCursor()
        : FGenericCursor()
    { }

    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(FGenericWindow* RelativeWindow, int32 x, int32 y) const override final;

    virtual void GetPosition(FGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    virtual void SetVisibility(bool bVisible) override final;
};