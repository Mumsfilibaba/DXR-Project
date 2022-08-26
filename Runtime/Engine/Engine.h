#pragma once
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"

#include "Application/Events.h"
#include "Application/User.h"
#include "Application/WindowMessageHandler.h"

#include "RHI/RHIViewport.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Resources/Material.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEngine - Class representing the engine

struct ENGINE_API FEngine
{
    FEngine();
    virtual ~FEngine() = default;

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
    virtual void Tick(FTimespan DeltaTime);

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
    FGenericWindowRef MainWindow;

    /** The main viewport */
    FRHIViewportRef MainViewport;

    /** User */
    TSharedPtr<FUser> User;

    /** The current scene */
    TSharedPtr<FScene> Scene;

    /** A completely white texture */
    FRHITexture2DRef BaseTexture;

    /** A completely flat normal map*/
    FRHITexture2DRef BaseNormal;

    /** Base sampler used by all materials */
    FRHISamplerStateRef BaseMaterialSampler;

    /** Base material */
    TSharedPtr<FMaterial> BaseMaterial;
};

extern ENGINE_API FEngine* GEngine;
