#pragma once
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Generic/GenericCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsCursor

class COREAPPLICATION_API FWindowsCursor final : public FGenericCursor
{
private:

    friend struct TDefaultDelete<FWindowsCursor>;

    FWindowsCursor()
        : FGenericCursor()
    { }

    ~FWindowsCursor() = default;

public:

    static FWindowsCursor* CreateWindowsCursor();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericCursor Interface

    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetVisibility(bool bIsVisible) override final;

    virtual void SetPosition(FGenericWindow* RelativeWindow, int32 x, int32 y)         const override final;
    virtual void GetPosition(FGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;
};
