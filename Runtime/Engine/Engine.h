#pragma once
#include "World/World.h"
#include "World/SceneViewport.h"
#include "Resources/Material.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h"

struct ENGINE_API FEngine
{
    FEngine();
    virtual ~FEngine();

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

    /** @brief - Creates the main window */
    void CreateMainWindow();

    /** @return - Returns true the main viewport could be initialized */
    bool CreateMainViewport();

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

    /** @brief - The current world */
    FWorld* World;

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
