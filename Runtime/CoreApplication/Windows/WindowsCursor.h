#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/CoreModule.h"
#include "CoreApplication/Interface/PlatformCursor.h"

class COREAPPLICATION_API CWindowsCursor final : public CPlatformCursor
{
    friend class CWindowsApplication;

public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the position of the cursor */
    virtual void SetPosition( CPlatformWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetCursorPosition( CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

private:

    FORCEINLINE CWindowsCursor()
        : CPlatformCursor()
    {
    }

    ~CWindowsCursor() = default;
};

#endif