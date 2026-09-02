#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/VisualElement.h"

class FGizmo;
class FViewport;
class FEditorViewportImage;

/** @brief Called for a click the gizmo and the game both declined, with the position inside the image. */
DECLARE_DELEGATE(FOnViewportClicked, const IntVector2& /*ImagePosition*/, bool /*bAdditive*/);

/** @brief Called for a right click that did not turn into a look drag, with the position inside the image and on the desktop. */
DECLARE_DELEGATE(FOnViewportContextMenu, const IntVector2& /*ImagePosition*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called when a left drag across the image is released, with the dragged corners inside the image. */
DECLARE_DELEGATE(FOnViewportMarqueeSelect, const IntVector2& /*ImageMin*/, const IntVector2& /*ImageMax*/, bool /*bAdditive*/);

/** @brief Called for a key the surface did not want itself, and only while a look drag is not underway. */
DECLARE_RETURN_DELEGATE(FOnViewportShortcut, bool, const FKeyEvent& /*KeyEvent*/);

class ENGINE_API FEditorViewportSurface final : public FVisualElement
{
public:

    /** @brief How far the cursor may travel with the right button held before the release stops being a click. */
    static constexpr float ContextMenuDragLimit = 4.0f;

    /** @brief How far the cursor must travel with the left button held before the press becomes a marquee instead of a click, in pixels. */
    static constexpr int32 MarqueeDragThreshold = 4;

public:
    static TSharedPtr<FEditorViewportSurface> Create();

public:
    FEditorViewportSurface();
    virtual ~FEditorViewportSurface();

    // FVisualElement Interface
    virtual void Tick(const FRectangle& AssignedBounds) override final;
    virtual IntVector2 ComputeDesiredSize() const override final;
    virtual void OnArrange(const FRectangle& AllottedBounds) override final;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override final;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override final;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final;

    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnFocusLost() override final;
    virtual bool SupportsKeyboardFocus() const override final;

    /**
     * @brief Sets the layers the surface arranges and draws, from the bottom up.
     *
     * @param InImage    The render target, drawn first and never interactive.
     * @param InViewport The element the game reads its input from, which declines everything while editing.
     * @param InGizmo    The transform handles, which get first refusal on every click.
     */
    void SetLayers(const TSharedPtr<FEditorViewportImage>& InImage, const TSharedPtr<FViewport>& InViewport, const TSharedPtr<FGizmo>& InGizmo);

    /** @brief Forgets any button held and any drag in progress, which entering or leaving play needs. */
    void ResetInputState();

    /**
     * @brief Sets whether the image is ringed to say the world is running rather than being authored.
     *
     * @param bInShowPlayBorder True to draw the ring.
     */
    void SetPlayBorderVisible(bool bInShowPlayBorder);

    /** @return The keys and buttons currently held, which the camera controller reads once per frame. */
    NODISCARD const struct FEditorCameraInputState& GetCameraInput() const;

    /** @brief Clears the per-frame deltas, leaving the held state alone. Called after the camera has read them. */
    void ConsumeCameraInputDeltas();

    /** @return True while the right button is held down inside the image, which is what a look drag is. */
    NODISCARD bool IsMouseLookActive() const
    {
        return bMouseLookActive;
    }

    FOnViewportClicked       OnClickedDelegate;
    FOnViewportContextMenu   OnContextMenuDelegate;
    FOnViewportMarqueeSelect OnMarqueeSelectDelegate;
    FOnViewportShortcut      OnShortcutDelegate;

private:
    NODISCARD bool IsGizmoBusy() const;
    NODISCARD FRectangle GetMarqueeRectangle() const;

    void EndMarquee();

    TSharedPtr<FEditorViewportImage>           Image;
    TSharedPtr<FViewport>                      HostViewport;
    TSharedPtr<FGizmo>                         Gizmo;
    TUniquePtr<struct FEditorCameraInputState> CameraInput;
    IntVector2                                 LastCursorPosition;
    IntVector2                                 MarqueeStartPosition;
    IntVector2                                 MarqueeEndPosition;
    float                                      RightMouseDragDistance;
    bool                                       bMouseLookActive;
    bool                                       bLeftPressedInside;
    bool                                       bMarqueeActive;
    bool                                       bShowPlayBorder;
};
