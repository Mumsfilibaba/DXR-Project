#pragma once
#include "Core/Math/Vector2.h"
#include "RHI/RHIResources.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FActor;
class FCameraComponent;

struct IEditorViewportHost
{
    virtual ~IEditorViewportHost() = default;

    /**
     * @brief Drops any reference the viewport holds to an actor the world is about to destroy.
     *
     * @param Actor The actor being removed.
     */
    virtual void OnActorRemoved(FActor* Actor) = 0;

    /**
     * @brief Delivers the pick a context-menu request asked for, which is what places a spawned actor.
     *
     * @param Result      What the renderer resolved, including the request id it was asked with.
     * @param PickedActor The actor the pick landed on, or null when it landed on nothing.
     */
    virtual void OnContextMenuPickResult(const FEditorPickResult& Result, FActor* PickedActor) = 0;

    /**
     * @brief Reads and clears the flag saying the camera jumped rather than moved.
     *
     * @return True when the next frame must not reproject history from the previous one.
     */
    virtual bool ConsumeCameraCut() = 0;

    /** @brief Releases mouse capture and clears any drag in progress, which entering or leaving play needs. */
    virtual void ResetInputState() = 0;

    /**
     * @brief Moves the camera so an actor fills the view, which is what activating a hierarchy row does.
     *
     * @param Actor The actor to frame, which may be null and is then ignored.
     */
    virtual void FocusOnActor(FActor* Actor) = 0;

    /**
     * @brief Points the viewport at the texture the scene now renders into.
     *
     * @param InViewportImage The render target, recreated whenever the viewport is resized.
     */
    virtual void SetViewportImage(FRHITextureRef InViewportImage) = 0;

    /** @return The size the scene should render at, in pixels, which is zero before the first layout. */
    virtual IntVector2 GetViewportSize() const = 0;

    /** @return The debug view the scene is drawn with, Nothing for the lit image. */
    virtual FSceneRenderView::EDebugView GetDebugView() const = 0;

    /** @return The debug view drawn in the split half, Nothing when the view is not split. */
    virtual FSceneRenderView::EDebugView GetSecondaryDebugView() const = 0;

    /** @return Which channels of the debug view are written, all of them by default. */
    virtual FSceneRenderView::EDebugViewChannel GetDebugViewChannelMask() const = 0;

    /** @return The camera the viewport looks through, or null while the editor has none. */
    virtual FCameraComponent* GetViewCamera() const = 0;
};
