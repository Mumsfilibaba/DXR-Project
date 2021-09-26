#pragma once
#include "Core.h"

#include "Core/Input/InputCodes.h"
#include "Core/Application/ModifierKeyState.h"
#include "Core/Application/Generic/GenericWindow.h"

struct SKeyEvent
{
	FORCEINLINE SKeyEvent( EKey InKeyCode, bool InIsDown, bool InIsRepeat, SModifierKeyState InModiferKeyState )
		: KeyCode( InKeyCode )
		, IsDown( InIsDown )
		, IsRepeat( InIsRepeat )
		, ModiferKeyState( InModiferKeyState )
	{
	}

	/* The KeyCode for this event */
	EKey KeyCode;
	
	/* If the key was down or nor */
	bool IsDown : 1;
	
	/* Is a repeated key event */
	bool IsRepeat : 1;
	
	/* The other modifer keys that where down at the same time as the event */
	SModifierKeyState ModiferKeyState;
};


struct SKeyTypedEvent
{
	FORCEINLINE SKeyTypedEvent( uint32 InCharacter )
        : Character( InCharacter )
    {
    }

    FORCEINLINE char GetPrintableCharacter() const
    {
        return static_cast<char>(Character);
    }

	/* The character that where pressed, this is a ascii character in most cases */
    uint32 Character;
};

struct SMouseMovedEvent
{
	FORCEINLINE SMouseMovedEvent( int32 InX, int32 InY )
        : x( InX )
        , y( InY )
    {
    }

    int32 x;
    int32 y;
};

struct SMouseButtonEvent
{
	FORCEINLINE SMouseButtonEvent( EMouseButton InButton, bool InIsDown, SModifierKeyState InModifiers )
        : Button( InButton )
		, IsDown( InIsDown )
        , Modifiers( InModifiers )
    {
    }

	/* The mouse button that for the event */
    EMouseButton Button;
	
	/* If the button where pressed or released */
	bool IsDown;
	
	/* The modifier keys that also where pressed */
    SModifierKeyState Modifiers;
};

struct SMouseScrolledEvent
{
	FORCEINLINE SMouseScrolledEvent( float InHorizontalDelta, float InVerticalDelta )
        : HorizontalDelta( InHorizontalDelta )
        , VerticalDelta( InVerticalDelta )
    {
    }

    float HorizontalDelta;
    float VerticalDelta;
};

struct SWindowResizeEvent
{
	FORCEINLINE SWindowResizeEvent( const TSharedRef<CGenericWindow>& InWindow, uint16 InWidth, uint16 InHeight )
        : Window( InWindow )
        , Width( InWidth )
        , Height( InHeight )
    {
    }

    TSharedRef<CGenericWindow> Window;
    uint16 Width;
    uint16 Height;
};

struct SWindowFocusChangedEvent
{
	FORCEINLINE SWindowFocusChangedEvent( const TSharedRef<CGenericWindow>& InWindow, bool hasFocus )
        : Window( InWindow )
        , HasFocus( hasFocus )
    {
    }

    TSharedRef<CGenericWindow> Window;
    bool HasFocus;
};

struct SWindowMovedEvent
{
	FORCEINLINE SWindowMovedEvent( const TSharedRef<CGenericWindow>& InWindow, int16 InX, int16 InY )
        : Window( InWindow )
        , x( InX )
		, y( InY )
    {
    }

    TSharedRef<CGenericWindow> Window;
	int16 x;
    int16 y;
};

struct SWindowFrameMouseEvent
{
	FORCEINLINE SWindowFrameMouseEvent( const TSharedRef<CGenericWindow>& InWindow, bool InMouseEntered )
        : Window( InWindow )
		, MouseEntered( InMouseEntered )
    {
    }

    TSharedRef<CGenericWindow> Window;
	bool MouseEntered;
};

struct SWindowClosedEvent
{
	FORCEINLINE SWindowClosedEvent( const TSharedRef<CGenericWindow>& InWindow )
        : Window( InWindow )
    {
    }

    TSharedRef<CGenericWindow> Window;
};
