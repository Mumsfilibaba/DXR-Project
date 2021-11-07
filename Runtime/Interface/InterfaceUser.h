#pragma once
#include "Events.h"
#include "InterfaceModule.h"

#include "Core/Input/InputStates.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Time/Timestamp.h"
#include "Core/Math/IntVector2.h"

#include "CoreApplication/ICursor.h"

/* Holds the state of one user */
class INTERFACE_API CInterfaceUser
{
    friend class CInterfaceApplication;

public:

    /* Create a new application user */
    static FORCEINLINE TSharedPtr<CInterfaceUser> Make( uint32 InUserIndex, ICursor* InCursorDevice )
    {
        return TSharedPtr<CInterfaceUser>( dbg_new CInterfaceUser( InUserIndex, InCursorDevice ) );
    }

    virtual ~CInterfaceUser();

    /* Tick the user every frame */
    virtual void Tick( CTimestamp DeltaTime );

    /* Called when user receives a key event */
    virtual void HandleKeyEvent( const SKeyEvent& KeyEvent );

    /* Called when the user receives a mouse button event */
    virtual void HandleMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent );

    /* Called when the mouse moved */
    virtual void HandleMouseMovedEvent( const SMouseMovedEvent& MouseMovedEvent );

    /* Called when the mouse scrolled */
    virtual void HandleMouseScrolledEvent( const SMouseScrolledEvent& MouseScolledEvent );

    /* Set the cursor position */
    virtual void SetCursorPosition( const CIntVector2& Postion );

    /* Retrieve the cursor position */
    virtual CIntVector2 GetCursorPosition() const;

    /* Retrieve the current key state */
    FORCEINLINE SKeyState GetKeyState( EKey KeyCode ) const
    {
        int32 Index = GetKeyStateIndexFromKeyCode( KeyCode );
        if ( Index < 0 )
        {
            return SKeyState( KeyCode );
        }
        else
        {
            return KeyStates[Index];
        }
    }

    /* Retrieve the current key state */
    FORCEINLINE SMouseButtonState GetMouseButtonState( EMouseButton Button ) const
    {
        int32 Index = GetMouseButtonStateIndexFromMouseButton( Button );
        if ( Index < 0 )
        {
            return SMouseButtonState( Button );
        }
        else
        {
            return MouseButtonStates[Index];
        }
    }

    /* Check if key is down */
    FORCEINLINE bool IsKeyDown( EKey KeyCode ) const
    {
        SKeyState KeyState = GetKeyState( KeyCode );
        return !!KeyState.IsDown;
    }

    /* Check if key is up */
    FORCEINLINE bool IsKeyUp( EKey KeyCode ) const
    {
        SKeyState KeyState = GetKeyState( KeyCode );
        return !KeyState.IsDown;
    }

    /* Check if key was pressed this frame */
    FORCEINLINE bool IsKeyPressed( EKey KeyCode ) const
    {
        SKeyState KeyState = GetKeyState( KeyCode );
        return KeyState.IsDown && !KeyState.PreviousState;
    }

    /* Check if button is down */
    FORCEINLINE bool IsMouseButtonDown( EMouseButton Button ) const
    {
        SMouseButtonState ButtonState = GetMouseButtonState( Button );
        return !!ButtonState.IsDown;
    }

    /* Check if key is up */
    FORCEINLINE bool IsMouseButtonUp( EMouseButton Button ) const
    {
        SMouseButtonState ButtonState = GetMouseButtonState( Button );
        return !ButtonState.IsDown;
    }

    /* Check if key was pressed this frame */
    FORCEINLINE bool IsMouseButtonPressed( EMouseButton Button ) const
    {
        SMouseButtonState ButtonState = GetMouseButtonState( Button );
        return ButtonState.IsDown && !ButtonState.PreviousState;
    }

    /* Retrieve the cursor */
    FORCEINLINE ICursor* GetCursorDevice() const
    {
        return Cursor;
    }

    /* Retrieve the user's index */
    FORCEINLINE uint32 GetUserIndex() const
    {
        return UserIndex;
    }

private:

    CInterfaceUser( uint32 InUserIndex, ICursor* InCursorDevice );

    /* Get the index in the key-state array */
    FORCEINLINE int32 GetKeyStateIndexFromKeyCode( EKey KeyCode ) const
    {
        SKeyState TempState( KeyCode );
        return KeyStates.Find( TempState, []( const SKeyState& LHS, const SKeyState& RHS ) -> bool
        {
            return (LHS.KeyCode == RHS.KeyCode);
        } );
    }

    /* Get the index in the key-state array */
    FORCEINLINE int32 GetMouseButtonStateIndexFromMouseButton( EMouseButton Button ) const
    {
        SMouseButtonState TempState( Button );
        return MouseButtonStates.Find( TempState, []( const SMouseButtonState& LHS, const SMouseButtonState& RHS ) -> bool
        {
            return (LHS.Button == RHS.Button);
        } );
    }

    /* The ID of this user */
    const uint32 UserIndex;

    /* The cursor that is controlled by this user */
    ICursor* Cursor;

    /* The key-state of this user */
    TArray<SKeyState> KeyStates; // TODO: Use a map instead? 

    /* The mouse button state  of this user */
    TArray<SMouseButtonState> MouseButtonStates; // TODO: Probably better to have this static since there is only a few buttons
};
