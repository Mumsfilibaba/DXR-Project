#pragma once
#include "Core.h"

#include "Core/Input/InputCodes.h"
#include "Core/Application/Events.h"
#include "Core/Application/ApplicationUser.h"
#include "Core/Application/WindowMessageHandler.h"
#include "Core/Delegates/Event.h"

#include "RenderLayer/Viewport.h"

// TODO: Later we should bind this to the viewport? 
class CEngineWindowHandler : public CWindowMessageHandler
{
public:

    DECLARE_DELEGATE( CWindowClosedDelegate, const SWindowClosedEvent& ClosedEvent );
    CWindowClosedDelegate WindowClosedDelegate;

    CEngineWindowHandler() = default;
    ~CEngineWindowHandler() = default;

    virtual bool OnWindowClosed( const SWindowClosedEvent& ClosedEvent ) override final
    {
        WindowClosedDelegate.Execute( ClosedEvent );
        return true;
    }
};

/* Class representing the engine */
class CEngine
{
public:

    /* Create a new engine instance */
    static FORCEINLINE TSharedPtr<CEngine> Make()
    {
        EngineInstance = TSharedPtr<CEngine>( DBG_NEW CEngine() );
        return EngineInstance;
    }

    /* Init engine instance from an existing instance */
    static FORCEINLINE TSharedPtr<CEngine> Make( const TSharedPtr<CEngine>& InEngineInstance )
    {
        EngineInstance = InEngineInstance;
        return EngineInstance;
    }

    /* Retrieve the engine instance */
    static FORCEINLINE CEngine& Get()
    {
        return *EngineInstance;
    }

    /* Public destructor for the TSharedPtr */
    virtual ~CEngine();

    /* Init engine */
    virtual bool Init();

    /* Release engine resources */
    virtual bool Release();

    /* Request exit from the engine */
    virtual void Exit();

    TSharedRef<CGenericWindow> MainWindow;
    TSharedRef<Viewport>       MainViewport;

    TSharedPtr<CApplicationUser> User;

    bool IsRunning = false;

private:

    CEngine();

    CEngineWindowHandler WindowHandler;

    /* Handle when the application is exited */
    void OnApplicationExit( int32 ExitCode );

    CDelegateHandle OnApplicationExitHandle;

    static TSharedPtr<CEngine> EngineInstance;
};