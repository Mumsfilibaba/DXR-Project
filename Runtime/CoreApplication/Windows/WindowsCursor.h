#pragma once

#if PLATFORM_WINDOWS
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Interface/PlatformCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsCursor

class COREAPPLICATION_API CWindowsCursor final : public CPlatformCursor
{
private:

    CWindowsCursor()
        : CPlatformCursor()
    { }

    ~CWindowsCursor() = default;

public:

    static TSharedPtr<CWindowsCursor> Make();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformCursor Interface

    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const override final;
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    virtual void SetVisibility(bool bIsVisible) override final;

};

#endif