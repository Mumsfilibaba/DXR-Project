#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Core/CoreCursorDevice.h"

class CWindowsCursorDevice final : public CCoreCursorDevice
{
    friend class CWindowsApplication;

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

    FORCEINLINE CWindowsCursorDevice()
        : CCoreCursorDevice()
    {
    }

    ~CWindowsCursorDevice() = default;
};

#endif