#pragma once
#include "Core/Input/InputCodes.h"

enum class ECursor
{
    None = 0,
    Arrow = 1,
    TextInput = 2,
    ResizeAll = 3,
    ResizeEW = 4,
    ResizeNS = 5,
    ResizeNESW = 6,
    ResizeNWSE = 7,
    Hand = 8,
    NotAllowed = 9,
};

class CCoreWindow;

class ICursorDevice
{
public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) = 0;

    /* Sets the position of the cursor */
    virtual void SetCursorPosition( CCoreWindow* RelativeWindow, int32 x, int32 y ) const = 0;

    /* Retrieve the cursor position of a window */
    virtual void GetCursorPosition( CCoreWindow* RelativeWindow, int32& OutX, int32& OutY ) const = 0;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) = 0;

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const = 0;

protected:
    ~ICursorDevice() = default;
};
