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
    static bool Create();
    static void Destroy();

    static FORCEINLINE bool IsInitialized()
    {
        return GEngine != nullptr;
    }

    static FORCEINLINE FEngine* Get()
    {
        return GEngine;
    }

public:
    FEngine();
    virtual ~FEngine();

    virtual bool Init();
    virtual void Release();

    virtual bool Start();
    virtual void Tick(float DeltaTime);
    virtual void RenderFrame();

    virtual void Exit() { }

    /** @brief Returns the current world */
    FWorld* GetWorld() const
    {
        return World;
    }

    /** @brief Returns the engine window */
    TSharedPtr<FWindowWidget> GetEngineWindow() const
    {
        return EngineWindow;
    }

	/** @brief Returns the engine window */
	TSharedPtr<FViewportWidget> GetViewportWidget() const
	{
		return EngineViewportWidget;
	}

    /** @brief Returns the SceneViewport */
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

    void OnEngineWindowClosed();
    void OnEngineWindowMoved(const FIntVector2& NewScreenPosition);
    void OnEngineWindowResized(const FIntVector2& NewScreenSize);

    TSharedPtr<FWindowWidget>   EngineWindow;
    TSharedPtr<FViewportWidget> EngineViewportWidget;
    TSharedPtr<FSceneViewport>  SceneViewport;
    
    FWorld*      World;
    FGameModule* GameModule;
    
#if ENGINE_DEBUG_INPUT
    TSharedPtr<FInputDebugInputHandler> InputDebugInputHandler;
#endif

    static FEngine* GEngine;
};
