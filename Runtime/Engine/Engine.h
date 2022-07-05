#pragma once
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"

#include "Canvas/Events.h"
#include "Canvas/User.h"
#include "Canvas/WindowMessageHandler.h"

#include "RHI/RHIViewport.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Resources/Material.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEngine - Class representing the engine

class ENGINE_API FEngine
{
protected:

    FEngine();
    virtual ~FEngine() = default;

public:

    /** @return: Creates and returns a new engine instance */
    static FEngine* CreateEngine();

    /**
     * @brief: Initialize the engine 
     * 
     * @return: Returns true if the initialization was successful
     */
    virtual bool Initialize();

    /**
     * @brief: Start the engine
     * 
     * @return: Returns true if the startup was successful
     */
    virtual bool Start();

    /**
     * @brief: Tick should be called once per frame 
     * 
     * @param DeltaTime: Time since the last tick
     */
    virtual void Tick(FTimestamp DeltaTime);

    /** 
     * Release engine resources
     * 
     * @return: Returns true if the release was successful
     */
    virtual bool Release();

    /** Request exit from the engine */
    void Exit();

    /** Destroy the engine */
    void Destroy();

    /** The main window of the app */
    FGenericWindowRef MainWindow;

    /** The main viewport */
    TSharedRef<FRHIViewport> MainViewport;

    /** User */
    TSharedPtr<FUser> User;

    /** The current scene */
    TSharedPtr<CScene> Scene;

    /** A completely white texture */
    TSharedRef<FRHITexture2D> BaseTexture;

    /** A completely flat normal map*/
    TSharedRef<FRHITexture2D> BaseNormal;

    /** Base sampler used by all materials */
    TSharedRef<FRHISamplerState> BaseMaterialSampler;

    /** Base material */
    TSharedPtr<CMaterial> BaseMaterial;
};

extern ENGINE_API FEngine* GEngine;
