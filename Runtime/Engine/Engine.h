#pragma once
#include "Scene/Scene.h"
#include "Scene/SceneViewport.h"
#include "Resources/Material.h"
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "RHI/RHIResources.h"

struct ENGINE_API FEngine
{
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

    /** @brief - The main viewport */
    TSharedRef<FSceneViewport> MainViewport;

    /** @brief - The current scene */
    FScene* Scene;

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
