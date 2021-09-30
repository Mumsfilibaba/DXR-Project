#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/Core/CoreCursorDevice.h"

class CMacCursorDevice final : public CCoreCursorDevice
{
    friend class CMacApplication;

public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the position of the cursor */
    virtual void SetCursorPosition( CCoreWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetCursorPosition( CCoreWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

private:

    FORCEINLINE CMacCursorDevice()
        : CCoreCursorDevice()
    {
    }

    ~CMacCursorDevice() = default;
};

#endif
