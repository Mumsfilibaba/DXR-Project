#pragma once
#include "Core/Input/InputCodes.h"

enum class ECursor
{
    None       = 0,
    Arrow      = 1,
    TextInput  = 2,
    ResizeAll  = 3,
    ResizeEW   = 4,
    ResizeNS   = 5,
    ResizeNESW = 6,
    ResizeNWSE = 7,
    Hand       = 8,
    NotAllowed = 9,
};

class CGenericWindow;

class ICursor
{
public:

    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) = 0;

    /* Sets the postion of the cursor */ 
    virtual void SetCursorPosition( CGenericWindow* RelativeWindow, int32 x, int32 y ) const = 0;

    /* Retrive the cursor position of a window */
    virtual void GetCursorPosition( CGenericWindow* RelativeWindow, int32& OutX, int32& OutY ) const = 0;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) = 0;

    /* Retrive the mouse visibility */
    virtual bool IsVisible() const = 0;
	
	/* Check if the current buttonstate is pressed */
	virtual bool IsButtonDown( EMouseButton Button ) const = 0;

	/* Check if the current buttonstate is released */
	virtual bool IsButtonUp( EMouseButton Button ) const  = 0;

protected:
    ~ICursor() = default;
};
