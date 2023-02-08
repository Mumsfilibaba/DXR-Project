#pragma once
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/GenericCursor.h"

class COREAPPLICATION_API FWindowsCursor final 
    : public FGenericCursor
{
public:
    FWindowsCursor()
        : FGenericCursor()
    { }

    virtual ~FWindowsCursor() = default;

    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetVisibility(bool bIsVisible) override final;

    virtual void SetPosition(int32 x, int32 y) const override final;

    virtual FIntVector2 GetPosition() const override final;
};
