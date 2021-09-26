#pragma once
#include "Core.h"

#include "Core/Input/InputCodes.h"
#include "Core/Application/Events.h"
#include "Core/Application/WindowMessageHandler.h"
#include "Core/Delegates/Event.h"

#include "RenderLayer/Viewport.h"

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

class CEngine
{
public:
	
	CEngine();
	~CEngine();
	
	/* Init engine */
    bool Init();

	/* Release engine resources */
    bool Release();

	/* Request exit from the engine */
    void Exit();
	
	static FORCEINLINE CEngine& Get()
	{
		return Instance;
	}
	
    TSharedRef<CGenericWindow> MainWindow;
    TSharedRef<Viewport>       MainViewport;

    bool IsRunning = false;

private:
	
	CEngineWindowHandler WindowHandler;
	
	/* Handle when the application is exited */
	void OnApplicationExit( int32 ExitCode );
	
	CDelegateHandle OnApplicationExitHandle;
		
	static CEngine Instance;
};
