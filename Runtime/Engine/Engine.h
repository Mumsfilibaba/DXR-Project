#pragma once
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"

#include "Application/Events.h"
#include "Application/User.h"
#include "Application/WindowMessageHandler.h"

#include "RHI/RHIResources.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Resources/Material.h"

struct ENGINE_API FEngine
{
    FEngine();
    virtual ~FEngine() = default;

    /**
     * @brief  - Initialize the engine 
     * @return - Returns true if the initialization was successful
     */
    virtual bool Initialize();

    /**
     * @brief  - Start the engine
     * @return - Returns true if the startup was successful
     */
    virtual bool Start();

    /**
     * @brief           - Tick should be called once per frame 
     * @param DeltaTime - Time since the last tick
     */
    virtual void Tick(FTimespan DeltaTime);

    /** 
     * @brief  - Release engine resources
     * @return - Returns true if the release was successful
     */
    virtual bool Release();

    /**
     * @brief - Request exit from the engine 
     */
    void Exit();

    /**
     * @brief - Destroy the engine 
     */
    void Destroy();

    /** @brief - The main window of the app */
    FGenericWindowRef MainWindow;

    /** @brief - The main viewport */
    FRHIViewportRef MainViewport;

    /** @brief - User */
    TSharedPtr<FUser> User;

    /** @brief - The current scene */
    TSharedPtr<FScene> Scene;

    /** @brief - A completely white texture */
    FRHITextureRef BaseTexture;

    /** @brief - A completely flat normal map*/
    FRHITextureRef BaseNormal;

    /** @brief - Base sampler used by all materials */
    FRHISamplerStateRef BaseMaterialSampler;

    /** @brief - Base material */
    TSharedPtr<FMaterial> BaseMaterial;
};

extern ENGINE_API FEngine* GEngine;
