#include "Application.h"

#include "Core/Input/InputStates.h"

TSharedPtr<CApplication> CApplication::ApplicationInstance;

CApplication::CApplication( const TSharedPtr<CGenericApplication>& InPlatformApplication )
    : CGenericApplicationMessageListener()
    , PlatformApplication( InPlatformApplication )
    , InputHandlers()
    , WindowMessageHandlers()
{
}

CApplication::~CApplication()
{
}

TSharedRef<CGenericWindow> CApplication::MakeWindow()
{
    return PlatformApplication->MakeWindow();
}

void CApplication::Tick( CTimestamp DeltaTime )
{
    const float Delta = static_cast<float>(DeltaTime.AsMilliSeconds());
    PlatformApplication->Tick( Delta );

    if ( !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->Tick( DeltaTime );
        }
    }
}

void CApplication::SetCursor( ECursor InCursor )
{
    ICursorDevice* Cursor = GetCursor();
    Cursor->SetCursor( InCursor );
}

void CApplication::SetCursorPosition( const CIntVector2& Position )
{
    ICursorDevice* Cursor = GetCursor();
    Cursor->SetCursorPosition( nullptr, Position.x, Position.y );
}

void CApplication::SetCursorPosition( const TSharedRef<CGenericWindow>& RelativeWindow, const CIntVector2& Position )
{
    ICursorDevice* Cursor = GetCursor();
    Cursor->SetCursorPosition( RelativeWindow.Get(), Position.x, Position.y );
}

CIntVector2 CApplication::GetCursorPosition() const
{
    ICursorDevice* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( nullptr, CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

CIntVector2 CApplication::GetCursorPosition( const TSharedRef<CGenericWindow>& RelativeWindow ) const
{
    ICursorDevice* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( RelativeWindow.Get(), CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

void CApplication::SetCursorVisibility( bool IsVisible )
{
    ICursorDevice* Cursor = GetCursor();
    Cursor->SetVisibility( IsVisible );
}

bool CApplication::IsCursorVisibile() const
{
    ICursorDevice* Cursor = GetCursor();
    return Cursor->IsVisible();
}

void CApplication::SetCapture( const TSharedRef<CGenericWindow>& CaptureWindow )
{
    PlatformApplication->SetCapture( CaptureWindow );
}

void CApplication::SetActiveWindow( const TSharedRef<CGenericWindow>& ActiveWindow )
{
    PlatformApplication->SetActiveWindow( ActiveWindow );
}

TSharedRef<CGenericWindow> CApplication::GetCapture() const
{
    return PlatformApplication->GetCapture();
}

TSharedRef<CGenericWindow> CApplication::GetActiveWindow() const
{
    return PlatformApplication->GetActiveWindow();
}

template<typename MessageHandlerType>
void CApplication::InsertMessageHandler( TArray<MessageHandlerType*>& OutMessageHandlerArray, MessageHandlerType* NewMessageHandler )
{
    if ( !OutMessageHandlerArray.Contains( NewMessageHandler ) )
    {
        const uint32 NewPriority = NewMessageHandler->GetPriority();
        for ( int32 Index = 0; Index < OutMessageHandlerArray.Size(); )
        {
            MessageHandlerType* Handler = OutMessageHandlerArray[Index];
            if ( NewPriority <= Handler->GetPriority() )
            {
                Index++;
            }
            else
            {
                OutMessageHandlerArray.Insert( Index, NewMessageHandler );
                break;
            }
        }

        OutMessageHandlerArray.Push( NewMessageHandler );
    }
}

void CApplication::AddInputHandler( CInputHandler* NewInputHandler )
{
    InsertMessageHandler( InputHandlers, NewInputHandler );
}

void CApplication::RemoveInputHandler( CInputHandler* InputHandler )
{
    InputHandlers.Remove( InputHandler );
}

void CApplication::AddWindowMessageHandler( CWindowMessageHandler* NewWindowMessageHandler )
{
    InsertMessageHandler( WindowMessageHandlers, NewWindowMessageHandler );
}

void CApplication::RemoveWindowMessageHandler( CWindowMessageHandler* InputHandler )
{
    WindowMessageHandlers.Remove( InputHandler );
}

void CApplication::SetPlatformApplication( const TSharedPtr<CGenericApplication>& InPlatformApplication )
{
    if ( InPlatformApplication )
    {
        InPlatformApplication->SetMessageListener( ApplicationInstance );
    }

    PlatformApplication = InPlatformApplication;
}

void CApplication::OnKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState )
{
    SKeyEvent KeyEvent( KeyCode, false, false, ModierKeyState );
    OnKeyEvent( KeyEvent );
}

void CApplication::OnKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState )
{
    SKeyEvent KeyEvent( KeyCode, true, IsRepeat, ModierKeyState );
    OnKeyEvent( KeyEvent );
}

void CApplication::OnKeyEvent( const SKeyEvent& KeyEvent )
{
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->OnKeyEvent( KeyEvent ) )
        {
            break;
        }
    }

    if ( !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleKeyEvent( KeyEvent );
        }
    }

    // TODO: Update viewport
}

void CApplication::OnKeyTyped( uint32 Character )
{
    SKeyTypedEvent KeyTypedEvent( Character );
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->OnKeyTyped( KeyTypedEvent ) )
        {
            break;
        }
    }
}

void CApplication::OnMouseMove( int32 x, int32 y )
{
    SMouseMovedEvent MouseMovedEvent( x, y );
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->OnMouseMove( MouseMovedEvent ) )
        {
            break;
        }
    }

    if ( !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleMouseMovedEvent( MouseMovedEvent );
        }
    }
}

void CApplication::OnMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState )
{
    TSharedRef<CGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
    if ( CaptureWindow )
    {
        PlatformApplication->SetCapture( nullptr );
    }

    SMouseButtonEvent MouseButtonEvent( Button, false, ModierKeyState );
    OnMouseButtonEvent( MouseButtonEvent );
}

void CApplication::OnMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState )
{
    TSharedRef<CGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
    if ( !CaptureWindow )
    {
        TSharedRef<CGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow();
        PlatformApplication->SetCapture( ActiveWindow );
    }

    SMouseButtonEvent MouseButtonEvent( Button, true, ModierKeyState );
    OnMouseButtonEvent( MouseButtonEvent );
}

void CApplication::OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent )
{
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->OnMouseButtonEvent( MouseButtonEvent ) )
        {
            break;
        }
    }

    if ( !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleMouseButtonEvent( MouseButtonEvent );
        }
    }
}

void CApplication::OnMouseScrolled( float HorizontalDelta, float VerticalDelta )
{
    SMouseScrolledEvent MouseScrolledEvent( HorizontalDelta, VerticalDelta );
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->OnMouseScrolled( MouseScrolledEvent ) )
        {
            break;
        }
    }

    if ( !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleMouseScrolledEvent( MouseScrolledEvent );
        }
    }
}

void CApplication::OnWindowResized( const TSharedRef<CGenericWindow>& Window, uint16 Width, uint16 Height )
{
    SWindowResizeEvent WindowResizeEvent( Window, Width, Height );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowResized( WindowResizeEvent ) )
        {
            break;
        }
    }
}

void CApplication::OnWindowMoved( const TSharedRef<CGenericWindow>& Window, int16 x, int16 y )
{
    SWindowMovedEvent WindowsMovedEvent( Window, x, y );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowMoved( WindowsMovedEvent ) )
        {
            break;
        }
    }
}

void CApplication::OnWindowFocusChanged( const TSharedRef<CGenericWindow>& Window, bool HasFocus )
{
    SWindowFocusChangedEvent WindowFocusChangedEvent( Window, HasFocus );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowFocusChanged( WindowFocusChangedEvent ) )
        {
            break;
        }
    }
}

void CApplication::OnWindowMouseLeft( const TSharedRef<CGenericWindow>& Window )
{
    SWindowFrameMouseEvent WindowFrameMouseEvent( Window, false );
    OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CApplication::OnWindowMouseEntered( const TSharedRef<CGenericWindow>& Window )
{
    SWindowFrameMouseEvent WindowFrameMouseEvent( Window, true );
    OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CApplication::OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& WindowFrameMouseEvent )
{
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowFrameMouseEvent( WindowFrameMouseEvent ) )
        {
            break;
        }
    }
}

void CApplication::OnWindowClosed( const TSharedRef<CGenericWindow>& Window )
{
    SWindowClosedEvent WindowClosedEvent( Window );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowClosed( WindowClosedEvent ) )
        {
            break;
        }
    }
}

void CApplication::OnApplicationExit( int32 ExitCode )
{
    ApplicationExitEvent.Broadcast( ExitCode );
}
