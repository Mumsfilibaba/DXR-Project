#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h"
#include "RendererCore/Interfaces/IRendererModule.h"
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
    static bool Initialize();
    static void Destroy();

    static FORCEINLINE bool IsInitialized()
    {
        return Engine != nullptr;
    }

    static FORCEINLINE FEngine* Get()
    {
        return Engine;
    }

public:
    FEngine();
    virtual ~FEngine();

    virtual bool Start();
    virtual bool Init();
    virtual bool InitPostRenderer() { return true; }
    virtual void Tick(float DeltaTime);
    virtual void Release();

    /**
     * @brief Build the by-value description of this frame's scene render (main thread).
     * The base fills the target swap-chain; subclasses fill the view (output target, debug views)
     * and any marshalled editor state (selection ObjectIDs).
     */
    virtual FSceneRenderPacket BuildRenderPacket();

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
    void OnEngineWindowMoved(const IntVector2& NewScreenPosition);
    void OnEngineWindowResized(const IntVector2& NewScreenSize);

    FWorld*                             World;
    FGameModule*                        GameModule;
    TSharedPtr<FWindowWidget>           EngineWindow;
    TSharedPtr<FViewportWidget>         EngineViewportWidget;
    TSharedPtr<FSceneViewport>          SceneViewport;
#if ENGINE_DEBUG_INPUT
    TSharedPtr<FInputDebugInputHandler> InputDebugInputHandler;
#endif

    static FEngine* Engine;
};
