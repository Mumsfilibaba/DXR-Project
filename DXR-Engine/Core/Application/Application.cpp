#include "Application.h"

#include "Core/Input/InputStates.h"

#include "Rendering/UIRenderer.h"

TSharedPtr<CApplication> CApplication::ApplicationInstance;

TSharedPtr<CApplication> CApplication::Make( const TSharedPtr<CCoreApplication>& InPlatformApplication )
{
    ApplicationInstance = TSharedPtr<CApplication>( DBG_NEW CApplication( InPlatformApplication ) );
    InPlatformApplication->SetMessageListener( ApplicationInstance );
    return ApplicationInstance;
}

/* Init the singleton from an existing application - Used for classes inheriting from CApplication */
TSharedPtr<CApplication> CApplication::Make( const TSharedPtr<CApplication>& InApplication )
{
    ApplicationInstance = InApplication;
    return ApplicationInstance;
}

void CApplication::Release()
{
    ApplicationInstance->SetPlatformApplication( nullptr );
    ApplicationInstance.Reset();
}

CApplication::CApplication( const TSharedPtr<CCoreApplication>& InPlatformApplication )
    : CCoreApplicationMessageHandler()
    , PlatformApplication( InPlatformApplication )
    , InputHandlers()
    , WindowMessageHandlers()
{
}

CApplication::~CApplication()
{
}

TSharedRef<CCoreWindow> CApplication::MakeWindow()
{
    return PlatformApplication->MakeWindow();
}

void CApplication::Tick( CTimestamp DeltaTime )
{
    CUIRenderer::Tick();

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
    ICursor* Cursor = GetCursor();
    Cursor->SetCursor( InCursor );
}

void CApplication::SetCursorPosition( const CIntVector2& Position )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetPosition( nullptr, Position.x, Position.y );
}

void CApplication::SetCursorPosition( const TSharedRef<CCoreWindow>& RelativeWindow, const CIntVector2& Position )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetPosition( RelativeWindow.Get(), Position.x, Position.y );
}

CIntVector2 CApplication::GetCursorPosition() const
{
    ICursor* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( nullptr, CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

CIntVector2 CApplication::GetCursorPosition( const TSharedRef<CCoreWindow>& RelativeWindow ) const
{
    ICursor* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( RelativeWindow.Get(), CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

void CApplication::SetCursorVisibility( bool IsVisible )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetVisibility( IsVisible );
}

bool CApplication::IsCursorVisibile() const
{
    ICursor* Cursor = GetCursor();
    return Cursor->IsVisible();
}

void CApplication::SetCapture( const TSharedRef<CCoreWindow>& CaptureWindow )
{
    PlatformApplication->SetCapture( CaptureWindow );
}

void CApplication::SetActiveWindow( const TSharedRef<CCoreWindow>& ActiveWindow )
{
    PlatformApplication->SetActiveWindow( ActiveWindow );
}

TSharedRef<CCoreWindow> CApplication::GetCapture() const
{
    return PlatformApplication->GetCapture();
}

TSharedRef<CCoreWindow> CApplication::GetActiveWindow() const
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

void CApplication::SetPlatformApplication( const TSharedPtr<CCoreApplication>& InPlatformApplication )
{
    if ( InPlatformApplication )
    {
        InPlatformApplication->SetMessageListener( ApplicationInstance );
    }

    PlatformApplication = InPlatformApplication;
}

void CApplication::HandleKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState )
{
    SKeyEvent KeyEvent( KeyCode, false, false, ModierKeyState );
    OnKeyEvent( KeyEvent );
}

void CApplication::HandleKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState )
{
    SKeyEvent KeyEvent( KeyCode, true, IsRepeat, ModierKeyState );
    OnKeyEvent( KeyEvent );
}

void CApplication::OnKeyEvent( const SKeyEvent& KeyEvent )
{
    SKeyEvent Event = KeyEvent;
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->HandleKeyEvent( Event ) )
        {
            Event.IsConsumed = true;
        }
    }

    if ( !Event.IsConsumed && !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleKeyEvent( Event );
        }
    }

    // TODO: Update viewport
}

void CApplication::HandleKeyTyped( uint32 Character )
{
    SKeyTypedEvent KeyTypedEvent( Character );
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->HandleKeyTyped( KeyTypedEvent ) )
        {
            KeyTypedEvent.IsConsumed = true;
        }
    }
}

void CApplication::HandleMouseMove( int32 x, int32 y )
{
    SMouseMovedEvent MouseMovedEvent( x, y );
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->HandleMouseMove( MouseMovedEvent ) )
        {
            MouseMovedEvent.IsConsumed = true;
        }
    }

    if ( !MouseMovedEvent.IsConsumed && !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleMouseMovedEvent( MouseMovedEvent );
        }
    }
}

void CApplication::HandleMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState )
{
    TSharedRef<CCoreWindow> CaptureWindow = PlatformApplication->GetCapture();
    if ( CaptureWindow )
    {
        PlatformApplication->SetCapture( nullptr );
    }

    SMouseButtonEvent MouseButtonEvent( Button, false, ModierKeyState );
    OnMouseButtonEvent( MouseButtonEvent );
}

void CApplication::HandleMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState )
{
    TSharedRef<CCoreWindow> CaptureWindow = PlatformApplication->GetCapture();
    if ( !CaptureWindow )
    {
        TSharedRef<CCoreWindow> ActiveWindow = PlatformApplication->GetActiveWindow();
        PlatformApplication->SetCapture( ActiveWindow );
    }

    SMouseButtonEvent MouseButtonEvent( Button, true, ModierKeyState );
    OnMouseButtonEvent( MouseButtonEvent );
}

void CApplication::OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent )
{
    SMouseButtonEvent Event = MouseButtonEvent;
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->HandleMouseButtonEvent( Event ) )
        {
            Event.IsConsumed = true;
        }
    }

    if ( !Event.IsConsumed && !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleMouseButtonEvent( Event );
        }
    }
}

void CApplication::HandleMouseScrolled( float HorizontalDelta, float VerticalDelta )
{
    SMouseScrolledEvent MouseScrolledEvent( HorizontalDelta, VerticalDelta );
    for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
    {
        CInputHandler* Handler = InputHandlers[Index];
        if ( Handler->HandleMouseScrolled( MouseScrolledEvent ) )
        {
            MouseScrolledEvent.IsConsumed = true;
        }
    }

    if ( !MouseScrolledEvent.IsConsumed && !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CApplicationUser>& User : RegisteredUsers )
        {
            User->HandleMouseScrolledEvent( MouseScrolledEvent );
        }
    }
}

void CApplication::HandleWindowResized( const TSharedRef<CCoreWindow>& Window, uint16 Width, uint16 Height )
{
    SWindowResizeEvent WindowResizeEvent( Window, Width, Height );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowResized( WindowResizeEvent ) )
        {
            WindowResizeEvent.IsConsumed = true;
        }
    }
}

void CApplication::HandleWindowMoved( const TSharedRef<CCoreWindow>& Window, int16 x, int16 y )
{
    SWindowMovedEvent WindowsMovedEvent( Window, x, y );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowMoved( WindowsMovedEvent ) )
        {
            WindowsMovedEvent.IsConsumed = true;
        }
    }
}

void CApplication::HandleWindowFocusChanged( const TSharedRef<CCoreWindow>& Window, bool HasFocus )
{
    SWindowFocusChangedEvent WindowFocusChangedEvent( Window, HasFocus );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowFocusChanged( WindowFocusChangedEvent ) )
        {
            WindowFocusChangedEvent.IsConsumed = true;
        }
    }
}

void CApplication::HandleWindowMouseLeft( const TSharedRef<CCoreWindow>& Window )
{
    SWindowFrameMouseEvent WindowFrameMouseEvent( Window, false );
    OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CApplication::HandleWindowMouseEntered( const TSharedRef<CCoreWindow>& Window )
{
    SWindowFrameMouseEvent WindowFrameMouseEvent( Window, true );
    OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CApplication::OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& WindowFrameMouseEvent )
{
    SWindowFrameMouseEvent Event = WindowFrameMouseEvent;
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowFrameMouseEvent( Event ) )
        {
            Event.IsConsumed = true;
        }
    }
}

void CApplication::HandleWindowClosed( const TSharedRef<CCoreWindow>& Window )
{
    SWindowClosedEvent WindowClosedEvent( Window );
    for ( int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++ )
    {
        CWindowMessageHandler* Handler = WindowMessageHandlers[Index];
        if ( Handler->OnWindowClosed( WindowClosedEvent ) )
        {
            WindowClosedEvent.IsConsumed = true;
        }
    }
}

void CApplication::HandleApplicationExit( int32 ExitCode )
{
    ApplicationExitEvent.Broadcast( ExitCode );
}
