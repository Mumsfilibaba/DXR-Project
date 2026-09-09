#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/World.h"
#include "Engine/World/SceneViewport.h"

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
     * @brief Enter run-mode, starting the world and handing control of the viewport to the game
     *
     * @return Returns true if the world went from being authored to running
     */
    virtual bool StartPlay();

    /**
     * @brief Leave run-mode, stopping the world and handing the viewport back
     */
    virtual void StopPlay();

    /**
     * @brief Freeze or resume a running world
     */
    void TogglePause();

    /** @brief Returns true while the world is running and advancing */
    bool IsPlaying() const
    {
        return World && World->IsPlaying();
    }

    /** @brief Returns true while the world is running but frozen */
    bool IsPaused() const
    {
        return World && World->IsPaused();
    }

    /** @brief Returns true while the world is being authored rather than run */
    bool IsEditing() const
    {
        return !World || World->IsEditing();
    }

    /**
     * @brief Build the by-value description of this frame's scene render (main thread).
     *
     * @return The packet, which a subclass fills with the view it wants rendered, the target it wants
     *         rendered into and any marshalled editor state.
     */
    virtual FSceneRenderPacket BuildRenderPacket();

    virtual void Exit() { }

    /** @return The texture the scene is rendered into, which the UI composites, or null before the first one exists. */
    NODISCARD FORCEINLINE FRHITexture* GetViewportImage() const
    {
        return ViewportImage.Get();
    }

    /** @brief Returns the current world */
    FWorld* GetWorld() const
    {
        return World;
    }

    /** @brief Returns the engine window */
    TSharedPtr<FWindow> GetEngineWindow() const
    {
        return EngineWindow;
    }

    /** @brief Returns the engine window */
    TSharedPtr<FViewport> GetViewport() const
    {
        return EngineViewport;
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

protected:
    /**
     * @brief Creates the render target the scene is drawn into at whatever size GetSceneRenderSize reports,
     * and hands it to the element that draws it. Called once during Init and again whenever that size
     * changes, so a caller only has to reach for it when driving the size from somewhere else.
     *
     * @return True when a target exists afterwards, including when a zero size left the previous one in place.
     */
    bool CreateViewportRenderTarget();

    /** @return The size the scene should render at, in pixels, which is zero while nothing has been laid out yet. */
    virtual IntVector2 GetSceneRenderSize() const;

    /**
     * @brief Points whatever draws the scene at the render target it was just given.
     *
     * @param InViewportImage The render target, recreated whenever the render size changes.
     */
    virtual void SetSceneRenderTarget(const FRHITextureRef& InViewportImage);

private:
    static constexpr EFormat ViewportImageFormat = EFormat::R8G8B8A8_Unorm;

    bool CreateEngineWindow();
    bool CreateEngineViewport();
    bool CreateSceneViewport();

    void OnEngineWindowClosed();
    void OnEngineWindowMoved(const IntVector2& NewScreenPosition);
    void OnEngineWindowResized(const IntVector2& NewScreenSize);

    FWorld*                    World;
    FGameModule*               GameModule;
    TSharedPtr<FWindow>        EngineWindow;
    TSharedPtr<FViewport>      EngineViewport;
    TSharedPtr<FSceneViewport> SceneViewport;
    FRHITextureRef             ViewportImage;
    IntVector2                 ViewportImageSize;

    static FEngine* Engine;
};
