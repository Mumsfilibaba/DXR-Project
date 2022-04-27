#pragma once
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Generic/GenericCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsCursor

class COREAPPLICATION_API CWindowsCursor final : public CGenericCursor
{
private:

    friend struct TDefaultDelete<CWindowsCursor>;

    CWindowsCursor()
        : CGenericCursor()
    { }

    ~CWindowsCursor() = default;

public:

    static TSharedPtr<CWindowsCursor> Make();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericCursor Interface

    virtual void SetCursor(ECursor Cursor)      override final;
    virtual void SetVisibility(bool bIsVisible) override final;

    virtual void SetPosition(CGenericWindow* RelativeWindow, int32 x, int32 y)         const override final;
    virtual void GetPosition(CGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;
};
