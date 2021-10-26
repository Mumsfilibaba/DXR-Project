#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/CoreAPI.h"
#include "Core/Application/Core/CoreCursor.h"

class CORE_API CWindowsCursor final : public CCoreCursor
{
    friend class CWindowsApplication;

public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the position of the cursor */
    virtual void SetPosition( CCoreWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetCursorPosition( CCoreWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

private:

    FORCEINLINE CWindowsCursor()
        : CCoreCursor()
    {
    }

    ~CWindowsCursor() = default;
};

#endif