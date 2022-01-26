#pragma once
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"

#include "Interface/Events.h"
#include "Interface/InterfaceUser.h"
#include "Interface/WindowMessageHandler.h"

#include "RHI/RHIViewport.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Resources/Material.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Engine - Class representing the engine

class ENGINE_API CEngine
{
public:

    /* Create a new engine instance */
    static CEngine* Make();

    /* Public destructor for the TSharedPtr */
    virtual ~CEngine() = default;

    /* Init engine */
    virtual bool Init();

    /* Start the engine */
    virtual bool Start();

    /* Tick should be called once per frame */
    virtual void Tick(CTimestamp DeltaTime);

    /* Release engine resources */
    virtual bool Release();

    /* Request exit from the engine */
    void Exit();

    /* The main window of the app */
    TSharedRef<CPlatformWindow> MainWindow;

    /* The main viewport */
    TSharedRef<CRHIViewport> MainViewport;

    // TODO: Remove
    TSharedPtr<CInterfaceUser> User;

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

protected:

    /* Engine should be constructed with the Make function */
    CEngine();
};

/* Global Engine Pointer */
extern ENGINE_API CEngine* GEngine;
