#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/World.h"
#include "Engine/World/SceneViewport.h"

#define ENGINE_DEBUG_INPUT 0

#if ENGINE_DEBUG_INPUT
struct FInputDebugInputHandler;
#endif

class ENGINE_API FEngine
{
public:

    /** @brief Initializes the Engine */
    static bool Create();

    /** @brief Releases the Engine */
    static void Destroy();

    /** @return Returns the true if the Engine is initialized */
    static FORCEINLINE bool IsInitialized()
    {
        return GEngine != nullptr;
    }

    /** @return Returns the Engine Interface */
    static FORCEINLINE FEngine* Get()
    {
        return GEngine;
    }

public:

    /** @brief Default constructor */
    FEngine();

    /** @brief Default destructor */
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
    virtual void Exit() { }

    // Returns the current world
    FWorld* GetWorld() const
    {
        return World;
    }

    // Returns the engine window
    TSharedPtr<FWindowWidget> GetEngineWindow() const
    {
        return EngineWindow;
    }

    // Returns the SceneViewport
    TSharedPtr<FSceneViewport> GetSceneViewport() const
    {
        return SceneViewport;
    }

    /** @brief A completely white texture */
    FRHITextureRef BaseTexture;

    /** @brief A completely flat normal map */
    FRHITextureRef BaseNormal;

    /** @brief Base sampler used by all materials */
    FRHISamplerStateRef BaseMaterialSampler;

    /** @brief Base material */
    TSharedPtr<FMaterial> BaseMaterial;

private:
    bool CreateEngineWindow();
    bool CreateEngineViewport();
    bool CreateSceneViewport();

    // Engine events
    void OnEngineWindowClosed();
    void OnEngineWindowMoved(const FIntVector2& NewScreenPosition);
    void OnEngineWindowResized(const FIntVector2& NewScreenSize);

    /** @brief The main Window */
    TSharedPtr<FWindowWidget> EngineWindow;

    /** @brief The main viewport */
    TSharedPtr<FViewportWidget> EngineViewportWidget;

    /** @brief SceneViewport */
    TSharedPtr<FSceneViewport> SceneViewport;

#if ENGINE_DEBUG_INPUT
    TSharedPtr<FInputDebugInputHandler> InputDebugInputHandler;
#endif

    /** @brief In-game Console Widget */
    TSharedPtr<class FInGameConsoleWidget> ConsoleWidget;

    /** @brief Profiler Widget */
    TSharedPtr<class FFrameProfilerWidget> ProfilerWidget;

    /** @brief Scene Inspector Widget */
    TSharedPtr<class FSceneInspectorWidget> InspectorWidget;

    /** @brief The current world */
    FWorld* World;

    /** @brief The current Game-Module */
    FGameModule* GameModule;

    static FEngine* GEngine;
};
