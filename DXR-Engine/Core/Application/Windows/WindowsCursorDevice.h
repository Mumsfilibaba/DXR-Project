#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Generic/GenericCursorDevice.h"

class CWindowsCursorDevice : public CGenericCursorDevice
{
    friend class CWindowsApplication;

public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the position of the cursor */
    virtual void SetCursorPosition( CGenericWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetCursorPosition( CGenericWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

private:

    FORCEINLINE CWindowsCursorDevice()
        : CGenericCursorDevice()
    {
    }

    ~CWindowsCursorDevice() = default;
};

#endif