#pragma once
#include "World/World.h"
#include "World/SceneViewport.h"
#include "Resources/Material.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h"

#define ENGINE_DEBUG_INPUT 0

#if ENGINE_DEBUG_INPUT
    struct FInputDebugInputHandler;
#endif

class ENGINE_API FEngine
{
public:
    FEngine();
    virtual ~FEngine();

    /** @return Returns true if Engine initialization was successful */
    virtual bool Init();

    /** @brief Releases engine resource */
    virtual void Release();

    /** @return Returns true if Starting the Engine was successful */
    virtual bool Start();

    /** @brief Tick the engine */
    virtual void Tick(float DeltaTime);
    
    /** @brief Exit the engine */
    virtual void Exit();

    /** @brief A completely white texture */
    FRHITextureRef BaseTexture;

    /** @brief A completely flat normal map */
    FRHITextureRef BaseNormal;

    /** @brief Base sampler used by all materials */
    FRHISamplerStateRef BaseMaterialSampler;

    /** @brief Base material */
    TSharedPtr<FMaterial> BaseMaterial;

    // Returns the current world
    FWorld* GetWorld() const { return World; }

    // Returns the engine window
    TSharedPtr<FWindow> GetEngineWindow() const { return EngineWindow; }

    // Returns the SceneViewport
    TSharedPtr<FSceneViewport> GetSceneViewport() const { return SceneViewport; }

private:
    bool CreateEngineWindow();
    bool CreateEngineViewport();
    bool CreateSceneViewport();

    // Engine events
    void OnEngineWindowClosed();
    void OnEngineWindowMoved(const FIntVector2& NewScreenPosition);
    void OnEngineWindowResized(const FIntVector2& NewScreenSize);

    /** @brief The main Window */
    TSharedPtr<FWindow> EngineWindow;

    /** @brief The main viewport */
    TSharedPtr<FViewport> EngineViewportWidget;

    /** @brief SceneViewport */
    TSharedPtr<FSceneViewport> SceneViewport;

#if ENGINE_DEBUG_INPUT
    TSharedPtr<FInputDebugInputHandler> InputDebugInputHandler;
#endif

    /** @brief In-game Console Widget */
    TSharedPtr<class FConsoleWidget> ConsoleWidget;

    /** @brief Profiler Widget */
    TSharedPtr<class FFrameProfilerWidget> ProfilerWidget;

    /** @brief Scene Inspector Widget */
    TSharedPtr<class FSceneInspectorWidget> InspectorWidget;

    /** @brief The current world */
    FWorld* World;

    /** @brief The current Game-Module */
    FGameModule* GameModule;
};

extern ENGINE_API FEngine* GEngine;
