#include "InterfaceApplication.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Core/Input/InputStates.h"

#include <imgui.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

TSharedPtr<CInterfaceApplication> CInterfaceApplication::Instance;

///////////////////////////////////////////////////////////////////////////////////////////////////

bool CInterfaceApplication::Make()
{
    /* Create the platform application */
    TSharedPtr<CPlatformApplication> Application = PlatformApplication::Make();
    if ( Application && !Application->Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create PlatformApplication" );
        return false;
    }

    Instance = TSharedPtr<CInterfaceApplication>( dbg_new CInterfaceApplication( Application ) );

    // Set the application to listen to messages from the platform application
    Application->SetMessageListener( Instance );

    return true;
}

/* Init the singleton from an existing application - Used for classes inheriting from CInterfaceApplication */
bool CInterfaceApplication::Make( const TSharedPtr<CInterfaceApplication>& InApplication )
{
    Instance = InApplication;
    return (Instance != nullptr);
}

void CInterfaceApplication::Release()
{
    Instance->SetPlatformApplication( nullptr );
    Instance.Reset();
}

CInterfaceApplication::CInterfaceApplication( const TSharedPtr<CPlatformApplication>& InPlatformApplication )
    : CPlatformApplicationMessageHandler()
    , PlatformApplication( InPlatformApplication )
    , InputHandlers()
    , WindowMessageHandlers()
    , RegisteredUsers()
    , Running( true )
    , DebugStrings()
{
}

TSharedRef<CPlatformWindow> CInterfaceApplication::MakeWindow()
{
    return PlatformApplication->MakeWindow();
}

void CInterfaceApplication::Tick( CTimestamp DeltaTime )
{
    // Update all the UI windows
    if ( Renderer )
    {
        Renderer->BeginTick();

        // Update all windows
        UIWindows.Foreach( []( const TSharedRef<IInterfaceWindow>& Window )
        {
            if ( Window->IsTickable() )
            {
                Window->Tick();
            }
        } );

        // Render all strings last
        RenderStrings();

        Renderer->EndTick();
    }

    // Update platform
    const float Delta = static_cast<float>(DeltaTime.AsMilliSeconds());
    PlatformApplication->Tick( Delta );

    if ( !RegisteredUsers.IsEmpty() )
    {
        for ( const TSharedPtr<CInterfaceUser>& User : RegisteredUsers )
        {
            User->Tick( DeltaTime );
        }
    }
}

void CInterfaceApplication::SetCursor( ECursor InCursor )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetCursor( InCursor );
}

void CInterfaceApplication::SetCursorPos( const CIntVector2& Position )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetPosition( nullptr, Position.x, Position.y );
}

void CInterfaceApplication::SetCursorPos( const TSharedRef<CPlatformWindow>& RelativeWindow, const CIntVector2& Position )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetPosition( RelativeWindow.Get(), Position.x, Position.y );
}

CIntVector2 CInterfaceApplication::GetCursorPos() const
{
    ICursor* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( nullptr, CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

CIntVector2 CInterfaceApplication::GetCursorPos( const TSharedRef<CPlatformWindow>& RelativeWindow ) const
{
    ICursor* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( RelativeWindow.Get(), CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

void CInterfaceApplication::ShowCursor( bool IsVisible )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetVisibility( IsVisible );
}

bool CInterfaceApplication::IsCursorVisibile() const
{
    ICursor* Cursor = GetCursor();
    return Cursor->IsVisible();
}

void CInterfaceApplication::SetCapture( const TSharedRef<CPlatformWindow>& CaptureWindow )
{
    PlatformApplication->SetCapture( CaptureWindow );
}

void CInterfaceApplication::SetActiveWindow( const TSharedRef<CPlatformWindow>& ActiveWindow )
{
    PlatformApplication->SetActiveWindow( ActiveWindow );
}

TSharedRef<CPlatformWindow> CInterfaceApplication::GetCapture() const
{
    return PlatformApplication->GetCapture();
}

TSharedRef<CPlatformWindow> CInterfaceApplication::GetActiveWindow() const
{
    return PlatformApplication->GetActiveWindow();
}

template<typename MessageHandlerType>
void CInterfaceApplication::InsertMessageHandler( TArray<MessageHandlerType*>& OutMessageHandlerArray, MessageHandlerType* NewMessageHandler )
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
                return;
            }
        }

        OutMessageHandlerArray.Push( NewMessageHandler );
    }
}

void CInterfaceApplication::AddInputHandler( CInputHandler* NewInputHandler )
{
    InsertMessageHandler( InputHandlers, NewInputHandler );
}

void CInterfaceApplication::RemoveInputHandler( CInputHandler* InputHandler )
{
    InputHandlers.Remove( InputHandler );
}

void CInterfaceApplication::RegisterMainViewport( const TSharedRef<CPlatformWindow>& NewMainViewport )
{
    MainViewport = NewMainViewport;
    if ( MainViewportChange.IsBound() )
    {
        MainViewportChange.Broadcast( MainViewport );
    }
}

void CInterfaceApplication::SetRenderer( const TSharedRef<IInterfaceRenderer>& NewRenderer )
{
    Renderer = NewRenderer;

    // Retrieve the context
    InterfaceContext NewContext = nullptr;
    if ( Renderer )
    {
        NewContext = Renderer->GetContext();
    }

    // Make sure that the context is initialized
    INIT_CONTEXT( NewContext );

    // Init all the windows
    UIWindows.Foreach( [=]( const TSharedRef<IInterfaceWindow>& Window )
    {
        Window->InitContext( NewContext );
    } );
}

void CInterfaceApplication::AddWindow( const TSharedRef<IInterfaceWindow>& NewWindow )
{
    if ( !UIWindows.Contains( NewWindow ) )
    {
        UIWindows.Emplace( NewWindow );
        if ( Renderer )
        {
            NewWindow->InitContext( Renderer->GetContext() );
        }
    }
}

void CInterfaceApplication::RemoveWindow( const TSharedRef<IInterfaceWindow>& Window )
{
    UIWindows.Remove( Window );
}

void CInterfaceApplication::DrawString( const CString& NewString )
{
    DebugStrings.Emplace( NewString );
}

void CInterfaceApplication::DrawWindows( CRHICommandList& CommandList )
{
    // NOTE: Renderer is not forced to be valid 
    if ( Renderer )
    {
        Renderer->Render( CommandList );
    }
}

void CInterfaceApplication::AddWindowMessageHandler( CWindowMessageHandler* NewWindowMessageHandler )
{
    InsertMessageHandler( WindowMessageHandlers, NewWindowMessageHandler );
}

void CInterfaceApplication::RemoveWindowMessageHandler( CWindowMessageHandler* InputHandler )
{
    WindowMessageHandlers.Remove( InputHandler );
}

void CInterfaceApplication::SetPlatformApplication( const TSharedPtr<CPlatformApplication>& InPlatformApplication )
{
    if ( InPlatformApplication )
    {
        Assert( this == Instance );
        InPlatformApplication->SetMessageListener( Instance );
    }

    PlatformApplication = InPlatformApplication;
}

void CInterfaceApplication::HandleKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState )
{
    SKeyEvent KeyEvent( KeyCode, false, false, ModierKeyState );
    OnKeyEvent( KeyEvent );
}

void CInterfaceApplication::HandleKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState )
{
    SKeyEvent KeyEvent( KeyCode, true, IsRepeat, ModierKeyState );
    OnKeyEvent( KeyEvent );
}

void CInterfaceApplication::OnKeyEvent( const SKeyEvent& KeyEvent )
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
        for ( const TSharedPtr<CInterfaceUser>& User : RegisteredUsers )
        {
            User->HandleKeyEvent( Event );
        }
    }

    // TODO: Update viewport
}

void CInterfaceApplication::HandleKeyTyped( uint32 Character )
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

void CInterfaceApplication::HandleMouseMove( int32 x, int32 y )
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
        for ( const TSharedPtr<CInterfaceUser>& User : RegisteredUsers )
        {
            User->HandleMouseMovedEvent( MouseMovedEvent );
        }
    }
}

void CInterfaceApplication::HandleMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState )
{
    TSharedRef<CPlatformWindow> CaptureWindow = PlatformApplication->GetCapture();
    if ( CaptureWindow )
    {
        PlatformApplication->SetCapture( nullptr );
    }

    SMouseButtonEvent MouseButtonEvent( Button, false, ModierKeyState );
    OnMouseButtonEvent( MouseButtonEvent );
}

void CInterfaceApplication::HandleMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState )
{
    TSharedRef<CPlatformWindow> CaptureWindow = PlatformApplication->GetCapture();
    if ( !CaptureWindow )
    {
        TSharedRef<CPlatformWindow> ActiveWindow = PlatformApplication->GetActiveWindow();
        PlatformApplication->SetCapture( ActiveWindow );
    }

    SMouseButtonEvent MouseButtonEvent( Button, true, ModierKeyState );
    OnMouseButtonEvent( MouseButtonEvent );
}

void CInterfaceApplication::OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent )
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
        for ( const TSharedPtr<CInterfaceUser>& User : RegisteredUsers )
        {
            User->HandleMouseButtonEvent( Event );
        }
    }
}

void CInterfaceApplication::HandleMouseScrolled( float HorizontalDelta, float VerticalDelta )
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
        for ( const TSharedPtr<CInterfaceUser>& User : RegisteredUsers )
        {
            User->HandleMouseScrolledEvent( MouseScrolledEvent );
        }
    }
}

void CInterfaceApplication::HandleWindowResized( const TSharedRef<CPlatformWindow>& Window, uint16 Width, uint16 Height )
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

void CInterfaceApplication::HandleWindowMoved( const TSharedRef<CPlatformWindow>& Window, int16 x, int16 y )
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

void CInterfaceApplication::HandleWindowFocusChanged( const TSharedRef<CPlatformWindow>& Window, bool HasFocus )
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

void CInterfaceApplication::HandleWindowMouseLeft( const TSharedRef<CPlatformWindow>& Window )
{
    SWindowFrameMouseEvent WindowFrameMouseEvent( Window, false );
    OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CInterfaceApplication::HandleWindowMouseEntered( const TSharedRef<CPlatformWindow>& Window )
{
    SWindowFrameMouseEvent WindowFrameMouseEvent( Window, true );
    OnWindowFrameMouseEvent( WindowFrameMouseEvent );
}

void CInterfaceApplication::OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& WindowFrameMouseEvent )
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

void CInterfaceApplication::RenderStrings()
{
    // Draw DebugWindow with DebugStrings
    if ( MainViewport && !DebugStrings.IsEmpty() )
    {
        SWindowShape CurrentWindowShape;
        MainViewport->GetWindowShape( CurrentWindowShape );

        constexpr float Width = 400.0f;
        ImGui::SetNextWindowPos( ImVec2( static_cast<float>(CurrentWindowShape.Width - Width), 18.0f ) );
        ImGui::SetNextWindowSize( ImVec2( Width, 0.0f ) );

        ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.3f, 0.3f, 0.3f, 0.6f ) );

        const ImGuiWindowFlags WindowFlags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin( "DebugWindow", nullptr, WindowFlags );
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

        for ( const CString& String : DebugStrings )
        {
            ImGui::Text( "%s", String.CStr() );
        }
        DebugStrings.Clear();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::End();
    }
}

void CInterfaceApplication::HandleWindowClosed( const TSharedRef<CPlatformWindow>& Window )
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

    // TODO: Register a main viewport and when that closes, request exit for now just exit
    PlatformApplicationMisc::RequestExit( 0 );
}

void CInterfaceApplication::HandleApplicationExit( int32 ExitCode )
{
    Running = false;
    ExitEvent.Broadcast( ExitCode );
}
