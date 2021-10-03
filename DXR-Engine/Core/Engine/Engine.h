#pragma once
#include "Core.h"

#include "Core/Input/InputCodes.h"
#include "Core/Application/Events.h"
#include "Core/Application/ApplicationUser.h"
#include "Core/Application/WindowMessageHandler.h"
#include "Core/Delegates/Event.h"

#include "RHICore/RHIViewport.h"

#include "Rendering/Resources/Material.h"

#include "Scene/Scene.h"

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
        return TSharedPtr<CEngine>( DBG_NEW CEngine() );;
    }

    /* Public destructor for the TSharedPtr */
    virtual ~CEngine();

    /* Init engine */
    virtual bool Init();

    /* Start the engine */
    virtual bool Start();

    /* Tick should be called once per frame */
    virtual void Tick( CTimestamp DeltaTime );

    /* Release engine resources */
    virtual bool Release();

    /* Request exit from the engine */
    virtual void Exit();

    /* The main window of the app */
    TSharedRef<CCoreWindow> MainWindow;

    /* The main viewport */
    TSharedRef<CRHIViewport> MainViewport;

    TSharedPtr<CApplicationUser> User;

    /* The current scene */
    TSharedPtr<CScene> Scene;

    /* A completely white texture */
    TSharedRef<CRHITexture2D> BaseTexture;

    /* A completely flat normal map*/
    TSharedRef<CRHITexture2D> BaseNormal;

    /* Base sampler used by all materials */
    TSharedRef<CRHISamplerState> BaseMaterialSampler;

    /* A completely white material */
    TSharedPtr<CMaterial> BaseMaterial;

    bool IsRunning = false;

private:

    CEngine();

    CEngineWindowHandler WindowHandler;

    /* Handle when the application is exited */
    void OnApplicationExit( int32 ExitCode );

    CDelegateHandle OnApplicationExitHandle;
};

extern TSharedPtr<CEngine> GEngine;