#pragma once

#if PLATFORM_WINDOWS
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Interface/PlatformCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for cursor

class COREAPPLICATION_API CWindowsCursor final : public CPlatformCursor
{
public:

    static TSharedPtr<CWindowsCursor> Make();

    /* Public constructor for the TSharedPtr*/
    ~CWindowsCursor() = default;

    /* Sets the type of cursor that is being used */
    virtual void SetCursor(ECursor Cursor) override final;

    /* Sets the position of the cursor */
    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility(bool bIsVisible) override final;

private:

    CWindowsCursor()
        : CPlatformCursor()
    {
    }
};

#endif