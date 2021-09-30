#include "Engine.h"

#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Debug/Console/Console.h"

/* Console vars */
ConsoleCommand GToggleFullscreen;
ConsoleCommand GExit;

/* Global engine instance */
TSharedPtr<CEngine> CEngine::EngineInstance;

CEngine::CEngine()
{
}

CEngine::~CEngine()
{
}

bool CEngine::Init()
{
    /* Register for events about exiting the application */
    OnApplicationExitHandle = CApplication::Get().ApplicationExitEvent.AddRaw( this, &CEngine::OnApplicationExit );

    /* Get notified when the main-window closes */
    WindowHandler.WindowClosedDelegate.BindLambda( [this]( const SWindowClosedEvent& ClosedEvent )
    {
        if ( this->MainWindow == ClosedEvent.Window )
        {
            this->Exit();
        }
    } );

    CApplication::Get().AddWindowMessageHandler( &WindowHandler );

    const uint32 Style =
        WindowStyleFlag_Titled |
        WindowStyleFlag_Closable |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    MainWindow = CApplication::Get().MakeWindow();
    if ( MainWindow && MainWindow->Init( "DXR Engine", 1920, 1080, Style ) )
    {
        MainWindow->Show( false );

        GToggleFullscreen.OnExecute.AddRaw( MainWindow.Get(), &CCoreWindow::ToggleFullscreen );
        INIT_CONSOLE_COMMAND( "a.ToggleFullscreen", &GToggleFullscreen );
    }
    else
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create Engine" );
        return false;
    }

    ICursorDevice* CursorDevice = CApplication::Get().GetCursor();
    User = CApplicationUser::Make( 0, CursorDevice );
    if ( !User )
    {
        return false;
    }

    CApplication::Get().RegisterUser( User );

    GExit.OnExecute.AddRaw( this, &CEngine::Exit );
    INIT_CONSOLE_COMMAND( "a.Exit", &GExit );

    IsRunning = true;
    return true;
}

bool CEngine::Release()
{
    /* Unregister for events */
    CApplication::Get().ApplicationExitEvent.Unbind( OnApplicationExitHandle );

    return true;
}

void CEngine::OnApplicationExit( int32 )
{
    IsRunning = false;
}

void CEngine::Exit()
{
    PlatformApplicationMisc::RequestExit( 0 );
}
