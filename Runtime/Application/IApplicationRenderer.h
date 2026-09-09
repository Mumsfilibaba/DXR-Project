#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Math/IntVector2.h"
#include "RHI/RHITypes.h"
#include "RHI/RHISwapChain.h"

class FWindow;
class FVisualElement;
class FDrawCommandList;

struct IApplicationRenderer
{
    /** @brief Virtual destructor for the IApplicationRenderer interface. */
    virtual ~IApplicationRenderer() = default;

    /**
     * @brief Opens the command list a window records into. The list belongs to the renderer, which keeps
     * it until the frame it was recorded for has been submitted, so nothing may hold on to it past the
     * matching EndWindow.
     *
     * @param InWindow The window about to be drawn.
     * @return The list to record into, or null when the renderer does not draw this window.
     */
    virtual FDrawCommandList* BeginWindow(const TSharedPtr<FWindow>& InWindow) = 0;

    /**
     * @brief Closes the list opened by the matching BeginWindow.
     *
     * @param InWindow The window that finished recording.
     */
    virtual void EndWindow(const TSharedPtr<FWindow>& InWindow) = 0;

    /**
     * @brief Releases whatever the renderer holds for a window that is going away.
     *
     * @param InWindow The window being destroyed.
     */
    virtual void OnWindowDestroyed(const TSharedPtr<FWindow>& InWindow) = 0;

    /**
     * @brief Draws an element and everything under it into a texture of its own, submitting the work
     * before returning so the texture can be sampled straight away. For a caller that wants a subtree as
     * a picture rather than as geometry, which is a snapshot taken once rather than a surface kept live.
     *
     * @param Element  The element to draw, which must already be arranged at the origin, since a subtree
     *                 draws into the rectangles a layout pass gave it rather than into one passed here.
     * @param Size     The logical size to cover, which the DPI scale multiplies into the texture's own.
     * @param DPIScale The scale the element was arranged at.
     * @return The texture, or null when there is no RHI to draw with or the subtree drew nothing.
     */
    virtual FRHITextureRef RenderElementToTexture(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, float DPIScale) = 0;

    /**
     * @brief Takes over the last reference to a texture a brush was drawn with, holding it the few frames it
     * takes for the lists already recording to reach the GPU. A draw command names its texture by raw
     * pointer, so a caller that releases one the frame after drawing it frees the object out from under a
     * barrier still queued behind it; handing it here instead is what makes replacing such a texture safe.
     *
     * @param Texture The texture to let go of, which may be null.
     */
    virtual void RetireTexture(const FRHITextureRef& Texture) = 0;

    /**
     * @brief Names the window the frame is paced and synchronised to, which the renderer gives a surface
     * straight away rather than at the start of the next frame, so whatever presents through it can be
     * wired up while the engine is still starting.
     *
     * @param InWindow The window, or null to leave every surface unpaced.
     */
    virtual void SetPrimaryWindow(const TSharedPtr<FWindow>& InWindow) = 0;

    /**
     * @brief Hands out the swap chain a window is presented through, which the renderer owns, so a caller
     * that has to name the same back buffer borrows it rather than creating a second one.
     *
     * @param InWindow The window to look up.
     * @return The swap chain, or null when the renderer holds no surface for the window.
     */
    virtual FRHISwapChainRef GetWindowSwapChain(const TSharedPtr<FWindow>& InWindow) const = 0;
};
