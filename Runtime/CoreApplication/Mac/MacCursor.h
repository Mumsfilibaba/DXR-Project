#pragma once

#if PLATFORM_MACOS
#include "CoreApplication/Interface/PlatformCursor.h"

class CMacCursor final : public CPlatformCursor
{
    friend class CMacApplication;

public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the position of the cursor */
    virtual void SetPosition( CPlatformWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetPosition( CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

private:

    FORCEINLINE CMacCursor()
        : CPlatformCursor()
    {
    }

    ~CMacCursor() = default;
};

#endif
