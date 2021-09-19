#include "Engine.h"

#include "Core/Application/Platform/Platform.h"
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Debug/Console/Console.h"

ConsoleCommand GToggleFullscreen;
ConsoleCommand GExit;

TSharedPtr<Engine> GEngine;

bool Engine::Init( class CGenericApplication* InApplication )
{
	Application = InApplication;

    const uint32 Style =
        WindowStyleFlag_Titled |
        WindowStyleFlag_Closable |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;
	
	MainWindow = Application->MakeWindow();
    if ( MainWindow && MainWindow->Init( "DXR Engine", 1920, 1080, Style ))
    {
        MainWindow->Show( false );

        GToggleFullscreen.OnExecute.AddRaw( MainWindow.Get(), &CGenericWindow::ToggleFullscreen );
        INIT_CONSOLE_COMMAND( "a.ToggleFullscreen", &GToggleFullscreen );
    }
    else
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create Engine" );
        return false;
    }

    GExit.OnExecute.AddRaw( this, &Engine::Exit );
    INIT_CONSOLE_COMMAND( "a.Exit", &GExit );

    IsRunning = true;
    return true;
}

bool Engine::Release()
{
	Application->SetMessageListener( nullptr );
	Application->Release();
    return true;
}

void Engine::Exit()
{
	PlatformApplicationMisc::RequestExit( 0 );
    IsRunning = false;
}

void Engine::OnKeyReleased( EKey KeyCode, SModifierKeyState ModfierKeyState )
{
    KeyReleasedEvent Event( KeyCode, ModfierKeyState );
    OnKeyReleasedEvent.Broadcast( Event );
}

void Engine::OnKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModfierKeyState )
{
    KeyPressedEvent Event( KeyCode, IsRepeat, ModfierKeyState );
    OnKeyPressedEvent.Broadcast( Event );
}

void Engine::OnKeyTyped( uint32 Character )
{
    KeyTypedEvent Event( Character );
    OnKeyTypedEvent.Broadcast( Event );
}

void Engine::OnMouseMove( int32 x, int32 y )
{
    MouseMovedEvent Event( x, y );
    OnMouseMoveEvent.Broadcast( Event );
}

void Engine::OnMouseReleased( EMouseButton Button, SModifierKeyState ModfierKeyState )
{
    CGenericWindow* CaptureWindow = Application->GetCapture();
    if ( CaptureWindow )
    {
        Platform::SetCapture( nullptr );
    }

    MouseReleasedEvent Event( Button, ModfierKeyState );
    OnMouseReleasedEvent.Broadcast( Event );
}

void Engine::OnMousePressed( EMouseButton Button, SModifierKeyState ModfierKeyState )
{
    CGenericWindow* CaptureWindow = Platform::GetCapture();
    if ( !CaptureWindow )
    {
        CGenericWindow* ActiveWindow = Platform::GetActiveWindow();
        Platform::SetCapture( ActiveWindow );
    }

    MousePressedEvent Event( Button, ModfierKeyState );
    OnMousePressedEvent.Broadcast( Event );
}

void Engine::OnMouseScrolled( float HorizontalDelta, float VerticalDelta )
{
    MouseScrolledEvent Event( HorizontalDelta, VerticalDelta );
    OnMouseScrolledEvent.Broadcast( Event );
}

void Engine::OnWindowResized( const TSharedRef<CGenericWindow>& InWindow, uint16 Width, uint16 Height )
{
    WindowResizeEvent Event( InWindow, Width, Height );
    OnWindowResizedEvent.Broadcast( Event );
}

void Engine::OnWindowMoved( const TSharedRef<CGenericWindow>& InWindow, int16 x, int16 y )
{
    WindowMovedEvent Event( InWindow, x, y );
    OnWindowMovedEvent.Broadcast( Event );
}

void Engine::OnWindowFocusChanged( const TSharedRef<CGenericWindow>& InWindow, bool HasFocus )
{
    WindowFocusChangedEvent Event( InWindow, HasFocus );
    OnWindowFocusChangedEvent.Broadcast( Event );
}

void Engine::OnWindowMouseLeft( const TSharedRef<CGenericWindow>& InWindow )
{
    WindowMouseLeftEvent Event( InWindow );
    OnWindowMouseLeftEvent.Broadcast( Event );
}

void Engine::OnWindowMouseEntered( const TSharedRef<CGenericWindow>& InWindow )
{
    WindowMouseEnteredEvent Event( InWindow );
    OnWindowMouseEnteredEvent.Broadcast( Event );
}

void Engine::OnWindowClosed( const TSharedRef<CGenericWindow>& InWindow )
{
    WindowClosedEvent Event( InWindow );
    OnWindowClosedEvent.Broadcast( Event );

    if ( InWindow == MainWindow )
    {
        PlatformApplicationMisc::RequestExit( 0 );
    }
}

void Engine::OnApplicationExit( int32 ExitCode )
{
    IsRunning = false;
    OnApplicationExitEvent.Broadcast( ExitCode );
}
