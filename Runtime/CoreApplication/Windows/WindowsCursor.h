#pragma once
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/GenericCursor.h"

class COREAPPLICATION_API FWindowsCursor final : public FGenericCursor
{
public:
    FWindowsCursor();
    virtual ~FWindowsCursor() = default;

    // ICursor Interface
    virtual void SetCursor(ECursor Cursor) override final;
    virtual void SetPosition(int32 x, int32 y) override final;
    virtual IntVector2 GetPosition() const override final;
    virtual void SetVisibility(bool bVisible) override final;
};
