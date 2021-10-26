#include "Application.h"

#include "Platform/PlatformApplication.h"
#include "Platform/PlatformApplicationMisc.h"

#include "Core/Input/InputStates.h"

#include <imgui.h>

/* Application */

TSharedPtr<CApplication> CApplication::Instance;

bool CApplication::Make()
{
    /* Create the platform application */
    TSharedPtr<CCoreApplication> Application = PlatformApplication::Make();
    if ( Application && !Application->Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create PlatformApplication" );
        return false;
    }

    Instance = TSharedPtr<CApplication>( dbg_new CApplication( Application ) );
    
    // Set the application to listen to messages from the platform application
    Application->SetMessageListener( Instance );

    return true;
}

/* Init the singleton from an existing application - Used for classes inheriting from CApplication */
bool CApplication::Make( const TSharedPtr<CApplication>& InApplication )
{
    Instance = InApplication;
    return (Instance != nullptr);
}

void CApplication::Release()
{
    Instance->SetPlatformApplication( nullptr );
    Instance.Reset();
}

CApplication::CApplication( const TSharedPtr<CCoreApplication>& InPlatformApplication )
    : CCoreApplicationMessageHandler()
    , PlatformApplication( InPlatformApplication )
    , InputHandlers()
    , WindowMessageHandlers()
    , RegisteredUsers()
    , Running( true )
    , DebugStrings()
{
}

TSharedRef<CCoreWindow> CApplication::MakeWindow()
{
    return PlatformApplication->MakeWindow();
}

void CApplication::Tick( CTimestamp DeltaTime )
{
    // Update all the UI windows
    if (Renderer)
    {
        Renderer->BeginTick();

        // Update all windows
        UIWindows.Foreach( []( const TSharedRef<IUIWindow>& Window )
        {
            if (Window->IsTickable())
            {
                Window->Tick();
            }
        });

        // Render all strings last
        RenderStrings();

        Renderer->EndTick();
    }

    // Update platform
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

void CApplication::SetCursorPos( const CIntVector2& Position )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetPosition( nullptr, Position.x, Position.y );
}

void CApplication::SetCursorPos( const TSharedRef<CCoreWindow>& RelativeWindow, const CIntVector2& Position )
{
    ICursor* Cursor = GetCursor();
    Cursor->SetPosition( RelativeWindow.Get(), Position.x, Position.y );
}

CIntVector2 CApplication::GetCursorPos() const
{
    ICursor* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( nullptr, CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

CIntVector2 CApplication::GetCursorPos( const TSharedRef<CCoreWindow>& RelativeWindow ) const
{
    ICursor* Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetCursorPosition( RelativeWindow.Get(), CursorPosition.x, CursorPosition.y );

    return CursorPosition;
}

void CApplication::ShowCursor( bool IsVisible )
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

void CApplication::RegisterMainViewport( const TSharedRef<CCoreWindow>& NewMainViewport )
{
    MainViewport = NewMainViewport;
    if ( MainViewportChange.IsBound())
    {
        MainViewportChange.Broadcast( MainViewport );
    }
}

void CApplication::SetRenderer( const TSharedRef<IUIRenderer>& NewRenderer )
{
    Renderer = NewRenderer;

    // Retrieve the context
    UIContextHandle NewContext = nullptr;
    if ( Renderer )
    {
        NewContext = Renderer->GetContext();
    }

    // Make sure that the context is initialized
    INIT_CONTEXT( NewContext );

    // Init all the windows
    UIWindows.Foreach([=]( const TSharedRef<IUIWindow>& Window )
    {
        Window->InitContext( NewContext );
    });
}

void CApplication::AddWindow( const TSharedRef<IUIWindow>& NewWindow )
{
    if (!UIWindows.Contains(NewWindow))
    {
        UIWindows.Emplace( NewWindow );
        if ( Renderer )
        {
            NewWindow->InitContext( Renderer->GetContext() );
        }
    }
}

void CApplication::RemoveWindow( const TSharedRef<IUIWindow>& Window )
{
    UIWindows.Remove( Window );
}

void CApplication::DrawString( const CString& NewString )
{
    DebugStrings.Emplace( NewString );
}

void CApplication::DrawWindows( CRHICommandList& CommandList )
{
    // NOTE: Renderer is not forced to be valid 
    if ( Renderer )
    {
        Renderer->Render( CommandList );
    }
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
        Assert( this == Instance );
        InPlatformApplication->SetMessageListener( Instance );
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

void CApplication::RenderStrings()
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

        ImGui::Begin( "DebugWindow", nullptr, WindowFlags);
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

    // TODO: Register a main viewport and when that closes, request exit for now just exit
    PlatformApplicationMisc::RequestExit( 0 );
}

void CApplication::HandleApplicationExit( int32 ExitCode )
{
    Running = false;
    ExitEvent.Broadcast( ExitCode );
}
