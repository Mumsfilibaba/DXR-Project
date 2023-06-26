#pragma once
#include "Scene/Scene.h"
#include "Scene/SceneViewport.h"
#include "Resources/Material.h"
#include "Core/Core.h"
#include "Core/Input/InputCodes.h"
#include "Core/Delegates/Event.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h"

struct ENGINE_API FEngine
{
    virtual ~FEngine() = default;

    /** @brief - Creates the main window */
    void CreateMainWindow();

    /** @return - Returns true the main viewport could be initialized */
    bool CreateMainViewport();

    /** @return - Returns true if Engine initialization was successful */
    virtual bool Init();

    /** @brief - Releases engine resource */
    virtual void Release();

    /** @return - Returns true if Starting the Engine was successful */
    virtual bool Start();

    /** @brief - Tick the engine */
    virtual void Tick(FTimespan DeltaTime);
    
    /** @brief - Exit the engine */
    virtual void Exit();

    /** @brief - The main Window */
    TSharedRef<FGenericWindow> MainWindow; 

    /** @brief - The main viewport */
    TSharedPtr<FViewport> MainViewport;

    /** @brief - SceneViewport */
    TSharedPtr<FSceneViewport> SceneViewport;

    /** @brief - In-game Console Widget */
    TSharedPtr<class FConsoleWidget> ConsoleWidget;

    /** @brief - Profiler Widget */
    TSharedPtr<class FFrameProfilerWidget> ProfilerWidget;

    /** @brief - The current scene */
    FScene* Scene;

    /** @brief - A completely white texture */
    FRHITextureRef BaseTexture;

    /** @brief - A completely flat normal map */
    FRHITextureRef BaseNormal;

    /** @brief - Base sampler used by all materials */
    FRHISamplerStateRef BaseMaterialSampler;

    /** @brief - Base material */
    TSharedPtr<FMaterial> BaseMaterial;
};

extern ENGINE_API FEngine* GEngine;
