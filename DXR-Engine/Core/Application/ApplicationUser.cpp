#include "ApplicationUser.h"
#include "MainApplication.h"

CApplicationUser::CApplicationUser( uint32 InUserIndex, ICursor* InCursorDevice )
    : UserIndex( InUserIndex )
    , CursorDevice( InCursorDevice )
    , KeyStates()
    , MouseButtonStates()
{
}

CApplicationUser::~CApplicationUser()
{
    // Empty for now
}

void CApplicationUser::Tick( CTimestamp DeltaTime )
{
    // Update all key-states 
    for ( SKeyState& KeyState : KeyStates )
    {
        if ( KeyState.IsDown )
        {
            KeyState.TimePressed += (float)DeltaTime.AsMilliSeconds();
        }
    }

    // Update all mouse-button states
    for ( SMouseButtonState& MouseButtonState : MouseButtonStates )
    {
        if ( MouseButtonState.IsDown )
        {
            MouseButtonState.TimePressed += (float)DeltaTime.AsMilliSeconds();
        }
    }
}

void CApplicationUser::HandleKeyEvent( const SKeyEvent& KeyEvent )
{
    int32 Index = GetKeyStateIndexFromKeyCode( KeyEvent.KeyCode );
    if ( Index >= 0 )
    {
        SKeyState& KeyState = KeyStates[Index];
        if ( KeyEvent.IsDown )
        {
            KeyState.PreviousState = KeyState.IsDown;
            KeyState.IsDown        = KeyEvent.IsDown;
            
            if ( KeyState.PreviousState )
            {
                KeyState.RepeatCount++;
            }
        }
        else
        {
            KeyState.IsDown        = 0;
            KeyState.PreviousState = 0;
            KeyState.RepeatCount   = 0;
            KeyState.TimePressed   = 0.0f;
        }
    }
    else
    {
        SKeyState NewKeyState( KeyEvent.KeyCode );
        KeyStates.Push( NewKeyState );
    }
}

void CApplicationUser::HandleMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent )
{
    int32 Index = GetMouseButtonStateIndexFromMouseButton( MouseButtonEvent.Button );
    if ( Index >= 0 )
    {
        SMouseButtonState& MouseButtonState = MouseButtonStates[Index];
        if ( MouseButtonState.IsDown )
        {
            MouseButtonState.PreviousState = MouseButtonState.IsDown;
            MouseButtonState.IsDown        = MouseButtonEvent.IsDown;
        }
        else
        {
            MouseButtonState.IsDown        = 0;
            MouseButtonState.PreviousState = 0;
            MouseButtonState.TimePressed   = 0.0f;
        }
    }
    else
    {
        SMouseButtonState NewMouseButtonState( MouseButtonEvent.Button );
        MouseButtonStates.Push( NewMouseButtonState );
    }
}

void CApplicationUser::HandleMouseMovedEvent( const SMouseMovedEvent& MouseMovedEvent )
{
    // TODO: Call all attached player controllers 
}

void CApplicationUser::HandleMouseScrolledEvent( const SMouseScrolledEvent& MouseScolledEvent )
{
    // TODO: Call all attached player controllers
}

void CApplicationUser::SetCursorPosition( const CIntPoint2& Postion )
{
    if (CursorDevice)
    {
        // TODO: Relative window
        CursorDevice->SetCursorPosition( nullptr, Postion.x, Postion.y );
    }
}

CIntPoint2 CApplicationUser::GetCursorPosition() const
{
    if ( CursorDevice )
    {
        CIntPoint2 Position;
        CursorDevice->GetCursorPosition( nullptr, Position.x, Position.y );
        return Position;
    }
    else
    {
        return CIntPoint2( -1, -1 );
    }
}
