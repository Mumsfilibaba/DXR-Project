#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/ICursor.h"

class CMacCursor final : public ICursor
{
	friend class CMacApplication;
	
public:

	/* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the postion of the cursor */
    virtual void SetCursorPosition( CGenericWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetCursorPosition( CGenericWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

    /* Retrieve the mouse visibility */
    virtual bool IsVisible() const override final
	{
		return IsCursorVisible;
	}

	/* Check if the current buttonstate is pressed */
	virtual bool IsButtonDown( EMouseButton Button ) const override final
	{
		return ButtonState[Button];
	}

	/* Check if the current buttonstate is released */
	virtual bool IsButtonUp( EMouseButton Button ) const override final
	{
		return !ButtonState[Button];
	}
	
	FORCEINLINE EMouseButton GetButtonFromIndex( uint32 ButtonIndex ) const
	{
		return ButtonFromButtonIndex[ButtonIndex];
	}
	
private:

	CMacCursor()  = default;
	~CMacCursor() = default;
	
	void InitLookUpTable();
	
	FORCEINLINE void RegisterButtonState( EMouseButton Button, bool State )
	{
		ButtonState[Button] = State;
	}
	
    /* Converts the buttonindex to enum */
	EMouseButton ButtonFromButtonIndex[EMouseButton::MouseButton_Count];
	
    /* The state of the mousebuttons */
    bool ButtonState[EMouseButton::MouseButton_Count];

    /* Checks if the mouse is visible or not */
    bool IsCursorVisible = true;
};

#endif
