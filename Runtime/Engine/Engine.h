#pragma once
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"

#include "Application/Events.h"
#include "Application/ApplicationUser.h"
#include "Application/WindowMessageHandler.h"

#include "RHI/RHIViewport.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Resources/Material.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CEngine - Class representing the engine

class ENGINE_API CEngine
{
public:

    /**
     * Create a new engine instance 
     * 
     * @return: Returns a new engine instance
     */
    static CEngine* CreateEngine();

    /**
     * Initialize the engine 
     * 
     * @return: Returns true if the initialization was successful
     */
    virtual bool Initialize();

    /**
     * Start the engine
     * 
     * @return: Returns true if the startup was successful
     */
    virtual bool Start();

    /**
     * Tick should be called once per frame 
     * 
     * @param DeltaTime: Time since the last tick
     */
    virtual void Tick(CTimestamp DeltaTime);

    /** 
     * Release engine resources
     * 
     * @return: Returns true if the release was successful
     */
    virtual bool Release();

    /**
     * Request exit from the engine 
     */
    void Exit();

    /**
     * Destroy the engine 
     */
    void Destroy();

    /** The main window of the app */
    TSharedRef<CPlatformWindow> MainWindow;

    /** The main viewport */
    TSharedRef<CRHIViewport> MainViewport;

    /** User */
    TSharedPtr<CApplicationUser> User;

    /** The current scene */
    TSharedPtr<CScene> Scene;

    /** A completely white texture */
    TSharedRef<CRHITexture2D> BaseTexture;

    /** A completely flat normal map*/
    TSharedRef<CRHITexture2D> BaseNormal;

    /** Base sampler used by all materials */
    TSharedRef<CRHISamplerState> BaseMaterialSampler;

    /** Base material */
    TSharedPtr<CMaterial> BaseMaterial;

protected:

    CEngine();
    virtual ~CEngine() = default;
};

/** Global Engine Pointer */
extern ENGINE_API CEngine* GEngine;
