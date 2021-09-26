#include "MainApplication.h"

#include "Core/Input/KeyState.h"

TSharedPtr<CMainApplication> CMainApplication::ApplicationInstance;

CMainApplication::CMainApplication( const TSharedPtr<CGenericApplication>& InPlatformApplication )
    : CGenericApplicationMessageListener()
    , PlatformApplication( InPlatformApplication )
	, InputHandlers()
	, WindowMessageHandlers()
{
}

CMainApplication::~CMainApplication()
{
}

TSharedRef<CGenericWindow> CMainApplication::MakeWindow()
{
	return PlatformApplication->MakeWindow();
}

void CMainApplication::Tick( CTimestamp DeltaTime )
{
	PlatformApplication->Tick( DeltaTime.AsMilliSeconds() );
}

void CMainApplication::SetCursor( ECursor InCursor )
{
	ICursor* Cursor = GetCursor();
	Cursor->SetCursor( InCursor );
}

void CMainApplication::SetCursorPosition( const CIntPoint2& Position )
{
	ICursor* Cursor = GetCursor();
	Cursor->SetCursorPosition( nullptr, Position.x, Position.y );
}

void CMainApplication::SetCursorPosition( const TSharedRef<CGenericWindow>& RelativeWindow, const CIntPoint2& Position )
{
	ICursor* Cursor = GetCursor();
	Cursor->SetCursorPosition( RelativeWindow.Get(), Position.x, Position.y );
}

CIntPoint2 CMainApplication::GetCursorPosition() const
{
	ICursor* Cursor = GetCursor();
	
	CIntPoint2 CursorPosition;
	Cursor->GetCursorPosition( nullptr, CursorPosition.x, CursorPosition.y );
	
	return CursorPosition;
}

CIntPoint2 CMainApplication::GetCursorPosition( const TSharedRef<CGenericWindow>& RelativeWindow ) const
{
	ICursor* Cursor = GetCursor();
	
	CIntPoint2 CursorPosition;
	Cursor->GetCursorPosition( RelativeWindow.Get(), CursorPosition.x, CursorPosition.y );
	
	return CursorPosition;
}

void CMainApplication::SetCursorVisibility( bool IsVisible )
{
	ICursor* Cursor = GetCursor();
	Cursor->SetVisibility( IsVisible );
}

bool CMainApplication::IsCursorVisibile() const
{
	ICursor* Cursor = GetCursor();
	return Cursor->IsVisible();
}

void CMainApplication::SetCapture( const TSharedRef<CGenericWindow>& CaptureWindow )
{
	PlatformApplication->SetCapture( CaptureWindow );
}

void CMainApplication::SetActiveWindow( const TSharedRef<CGenericWindow>& ActiveWindow )
{
	PlatformApplication->SetActiveWindow( ActiveWindow );
}

TSharedRef<CGenericWindow> CMainApplication::GetCapture() const
{
	return PlatformApplication->GetCapture();
}

TSharedRef<CGenericWindow> CMainApplication::GetActiveWindow() const
{
	return PlatformApplication->GetActiveWindow();
}

template<typename MessageHandlerType>
void CMainApplication::InsertMessageHandler( TArray<MessageHandlerType*>& OutMessageHandlerArray, MessageHandlerType* NewMessageHandler )
{
	if ( !OutMessageHandlerArray.Contains( NewMessageHandler ) )
	{
		const uint32 NewPriority = NewMessageHandler->GetPriority();
		for ( int32 Index = 0; Index < OutMessageHandlerArray.Size(); )
		{
			MessageHandlerType* Handler = OutMessageHandlerArray[Index];
			if ( NewPriority <= Handler->GetPriority())
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

void CMainApplication::AddInputHandler( CInputHandler* NewInputHandler )
{
	InsertMessageHandler( InputHandlers, NewInputHandler );
}

void CMainApplication::RemoveInputHandler( CInputHandler* InputHandler )
{
	InputHandlers.Remove( InputHandler );
}

void CMainApplication::AddWindowMessageHandler( CWindowMessageHandler* NewWindowMessageHandler )
{
	InsertMessageHandler( WindowMessageHandlers, NewWindowMessageHandler );
}

void CMainApplication::RemoveWindowMessageHandler( CWindowMessageHandler* InputHandler )
{
	WindowMessageHandlers.Remove( InputHandler );
}

void CMainApplication::SetPlatformApplication( const TSharedPtr<CGenericApplication>& InPlatformApplication )
{
	if ( InPlatformApplication )
	{
		InPlatformApplication->SetMessageListener( ApplicationInstance );
	}
	
	PlatformApplication = InPlatformApplication;
}

void CMainApplication::OnKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState )
{
	SKeyEvent KeyEvent( KeyCode, false, false, ModierKeyState );
	OnKeyEvent( KeyEvent );
}

void CMainApplication::OnKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState )
{
	SKeyEvent KeyEvent( KeyCode, true, IsRepeat, ModierKeyState );
	OnKeyEvent( KeyEvent );
}

void CMainApplication::OnKeyEvent( const SKeyEvent& KeyEvent )
{
	for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
	{
		CInputHandler* Handler = InputHandlers[Index];
		if ( Handler->OnKeyEvent( KeyEvent ) )
		{
			break;
		}
	}
	
	// TODO: Update global keystates
	
	// TODO: Update viewport
}

void CMainApplication::OnKeyTyped( uint32 Character )
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

void CMainApplication::OnMouseMove( int32 x, int32 y )
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
}

void CMainApplication::OnMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState )
{
	TSharedRef<CGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
	if ( CaptureWindow )
	{
		PlatformApplication->SetCapture( nullptr );
	}
	
	SMouseButtonEvent MouseButtonEvent( Button, false, ModierKeyState );
	OnMouseButtonEvent( MouseButtonEvent );
}

void CMainApplication::OnMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState )
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

void CMainApplication::OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent )
{
	for ( int32 Index = 0; Index < InputHandlers.Size(); Index++ )
	{
		CInputHandler* Handler = InputHandlers[Index];
		if ( Handler->OnMouseButtonEvent( MouseButtonEvent ) )
		{
			break;
		}
	}
}

void CMainApplication::OnMouseScrolled( float HorizontalDelta, float VerticalDelta )
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
}

void CMainApplication::OnWindowResized( const TSharedRef<CGenericWindow>& Window, uint16 Width, uint16 Height )
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

void CMainApplication::OnWindowMoved( const TSharedRef<CGenericWindow>& Window, int16 x, int16 y )
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

void CMainApplication::OnWindowFocusChanged( const TSharedRef<CGenericWindow>& Window, bool HasFocus )
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

void CMainApplication::OnWindowMouseLeft( const TSharedRef<CGenericWindow>& Window )
{
	SWindowFrameMouseEvent WindowFrameMouseEvent( Window, false );
	OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CMainApplication::OnWindowMouseEntered( const TSharedRef<CGenericWindow>& Window )
{
	SWindowFrameMouseEvent WindowFrameMouseEvent( Window, true );
	OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CMainApplication::OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& WindowFrameMouseEvent )
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

void CMainApplication::OnWindowClosed( const TSharedRef<CGenericWindow>& Window )
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

void CMainApplication::OnApplicationExit( int32 ExitCode )
{
	ApplicationExitEvent.Broadcast( ExitCode );
}
