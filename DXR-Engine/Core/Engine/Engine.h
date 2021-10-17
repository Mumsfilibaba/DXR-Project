#pragma once
#include "Core/CoreAPI.h"
#include "Core/Input/InputCodes.h"
#include "Core/Application/Events.h"
#include "Core/Application/ApplicationUser.h"
#include "Core/Application/WindowMessageHandler.h"
#include "Core/Delegates/Event.h"

#include "CoreRHI/RHIViewport.h"

#include "Rendering/Resources/Material.h"

#include "Scene/Scene.h"

/* Class representing the engine */
class CORE_API CEngine
{
public:

    /* Create a new engine instance */
    static FORCEINLINE TSharedPtr<CEngine> Make()
    {
        return TSharedPtr<CEngine>( DBG_NEW CEngine() );;
    }

    /* Public destructor for the TSharedPtr */
    virtual ~CEngine() = default;

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
};

extern CORE_API TSharedPtr<CEngine> GEngine;