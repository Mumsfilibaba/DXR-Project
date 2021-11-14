#include "InterfaceUser.h"
#include "InterfaceApplication.h"

CInterfaceUser::CInterfaceUser( uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor )
    : UserIndex( InUserIndex )
    , Cursor( InCursor )
    , KeyStates()
    , MouseButtonStates()
{
}

CInterfaceUser::~CInterfaceUser()
{
    // Empty for now
}

void CInterfaceUser::Tick( CTimestamp DeltaTime )
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

void CInterfaceUser::HandleKeyEvent( const SKeyEvent& KeyEvent )
{
    int32 Index = GetKeyStateIndexFromKeyCode( KeyEvent.KeyCode );
    if ( Index >= 0 )
    {
        SKeyState& KeyState = KeyStates[Index];
        if ( KeyEvent.IsDown )
        {
            KeyState.PreviousState = KeyState.IsDown;
            KeyState.IsDown = KeyEvent.IsDown;

            if ( KeyEvent.IsRepeat )
            {
                KeyState.RepeatCount++;
            }
        }
        else
        {
            KeyState.IsDown = 0;
            KeyState.PreviousState = 0;
            KeyState.RepeatCount = 0;
            KeyState.TimePressed = 0.0f;
        }
    }
    else
    {
        SKeyState NewKeyState( KeyEvent.KeyCode );
        NewKeyState.IsDown = KeyEvent.IsDown;
        KeyStates.Push( NewKeyState );
    }
}

void CInterfaceUser::HandleMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent )
{
    int32 Index = GetMouseButtonStateIndexFromMouseButton( MouseButtonEvent.Button );
    if ( Index >= 0 )
    {
        SMouseButtonState& MouseButtonState = MouseButtonStates[Index];
        if ( MouseButtonState.IsDown )
        {
            MouseButtonState.PreviousState = MouseButtonState.IsDown;
            MouseButtonState.IsDown = MouseButtonEvent.IsDown;
        }
        else
        {
            MouseButtonState.IsDown = 0;
            MouseButtonState.PreviousState = 0;
            MouseButtonState.TimePressed = 0.0f;
        }
    }
    else
    {
        SMouseButtonState NewMouseButtonState( MouseButtonEvent.Button );
        NewMouseButtonState.IsDown = MouseButtonEvent.IsDown;
        MouseButtonStates.Push( NewMouseButtonState );
    }
}

void CInterfaceUser::HandleMouseMovedEvent( const SMouseMovedEvent& MouseMovedEvent )
{
    UNREFERENCED_VARIABLE( MouseMovedEvent );
    // TODO: Call all attached player controllers 
}

void CInterfaceUser::HandleMouseScrolledEvent( const SMouseScrolledEvent& MouseScolledEvent )
{
    UNREFERENCED_VARIABLE( MouseScolledEvent );
    // TODO: Call all attached player controllers
}

void CInterfaceUser::SetCursorPosition( const CIntVector2& Postion )
{
    if ( Cursor )
    {
        // TODO: Relative window
        Cursor->SetPosition( nullptr, Postion.x, Postion.y );
    }
}

CIntVector2 CInterfaceUser::GetCursorPosition() const
{
    if ( Cursor )
    {
        CIntVector2 Position;
        Cursor->GetPosition( nullptr, Position.x, Position.y );
        return Position;
    }
    else
    {
        return CIntVector2( -1, -1 );
    }
}
