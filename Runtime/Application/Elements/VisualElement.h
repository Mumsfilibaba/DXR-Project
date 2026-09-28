#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Events.h"
#include "Application/Layout/LayoutTypes.h"
#include "CoreApplication/PlatformInterface/IPlatformCursor.h"

class FElementPath;
class FDrawCommandList;
class FScrollBox;
struct FDrawGeometry;
struct FDrawCacheBlock;

enum class EVisibility
{
    /** @brief No visibility flags set. */
    None = 0,
    
    /** @brief Element is hidden. */
    Hidden = BIT(1),
    
    /** @brief Element is visible. */
    Visible = BIT(2), 
};

ENUM_CLASS_OPERATORS(EVisibility);

enum class EElementActivationPolicy
{
    /** @brief When the owning window is activated, focus the window content element. */
    AutoFocusOnWindowActivate,

    /** @brief Do not automatically focus this element when the owning window is activated. */
    DoNotAutoFocusOnWindowActivate,
};

enum class EDrawCachePolicy : uint8
{
    /** @brief Let the heuristic decide, promoting the subtree once it is both large enough and stable. */
    Auto,

    /**
     * @brief Never cache, for an element whose look follows something no mutation of the tree announces: a
     * caret blinking off wall-clock time, a histogram or log view fed every frame from a live source, a
     * viewport or gizmo that follows the scene camera, a canvas being panned under the cursor. A Never
     * element also blocks captures by its ancestors, so the cache root settles below it.
     */
    Never,

    /** @brief Cache as soon as the subtree is clean, whatever size the heuristic would have asked for. */
    Always,
};

class APPLICATION_API FVisualElement : public TSharedFromThis<FVisualElement>
{
public:
    FVisualElement();
    virtual ~FVisualElement();

    /** @brief Stores the assigned bounds as the content rectangle and then arranges the children inside it. */
    virtual void Tick(const FRectangle& AssignedBounds);

    /**
     * @brief Checks if the element is a window.
     *
     * @return True if the element is an FWindow, false otherwise.
     */
    virtual bool IsWindow() const;

    /**
     * @brief Whether the element answers mouse input rather than only drawing.
     *
     * A title bar asks this of its descendants to work out which parts of the caption stay clickable
     * instead of dragging the window.
     * @return True if the element is an FInteractiveElement, false otherwise.
     */
    virtual bool IsInteractive() const;

    /**
     * @brief Whether this element takes every mouse event in its window while it is up.
     *
     * @return True while the element is modal over its window.
     */
    virtual bool CapturesAllInput() const;

    /** @brief Whether a click on this element should hand it the keyboard. The default is false. */
    virtual bool SupportsKeyboardFocus() const;

    /**
     * @brief Whether a keystroke reaching this element would be typed into it, which is what tells an
     * application-level shortcut to stand aside rather than swallow the character.
     *
     * @return True for a field being edited, and false for everything else.
     */
    virtual bool WantsTextInput() const;

    /** @brief The element that takes the keyboard when this one is focused, which is itself unless overridden. */
    virtual TSharedPtr<FVisualElement> GetFocusTarget();

    /**
     * @brief Handles analog gamepad input changes.
     * 
     * @param AnalogGamepadEvent The analog gamepad event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent);

    /**
     * @brief Handles key down events.
     * 
     * @param KeyEvent The key event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent);

    /**
     * @brief Handles key up events.
     * 
     * @param KeyEvent The key event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent);

    /**
     * @brief Handles character input events.
     * 
     * @param KeyEvent The key event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent);

    /**
     * @brief Handles mouse movement events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse button down events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse button up events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse scroll events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse double-click events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse leave events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse enter events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles high-precision mouse input events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent);

    /**
     * @brief The shape the cursor takes over this element.
     *
     * @param OutCursor Receives the shape when the element has an opinion.
     * @return True when the element has an opinion, false to leave the shape to its children.
     */
    virtual bool GetCursor(ECursor& OutCursor) const;

    /**
     * @brief Handles focus lost events.
     * 
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnFocusLost();

    /**
     * @brief Handles focus gained events.
     * 
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnFocusGained();

    /**
     * @brief Computes the size this element wants, ignoring the space it will actually be given.
     *
     * @return The desired size in pixels, which is zero for an element that takes no part in layout.
     */
    virtual IntVector2 ComputeDesiredSize() const;

    /**
     * @brief Arranges the children inside the rectangle this element was given.
     *
     * @param AllottedBounds The rectangle this element was given.
     */
    virtual void OnArrange(const FRectangle& AllottedBounds);

    /**
     * @brief Appends the direct children of this element in front to back order.
     *
     * @param OutChildren The array to append to.
     */
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const;

    /**
     * @brief Appends the draw commands for this element and its children.
     *
     * A container calls Draw on its children rather than this, so that a clean subtree can be replayed from
     * what it recorded before.
     *
     * @param AllottedGeometry The rectangle and scale the element was arranged into.
     * @param OutCommandList   The list to append to.
     * @param LayerId          The layer this element draws on.
     * @return The highest layer this element or any descendant drew on.
     */
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const;

    /**
     * @brief Records the element into the command list, replaying what it recorded before when nothing that
     * would change the result has moved since, so that the commands appended and the layer returned always
     * match what OnDraw alone would give.
     *
     * @param AllottedGeometry The rectangle and scale the element was arranged into.
     * @param OutCommandList   The list to append to.
     * @param LayerId          The layer this element draws on.
     * @return The highest layer this element or any descendant drew on.
     */
    int32 Draw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const;

    /**
     * @brief Rounds the outline the element draws around itself, which a popup sets on its content when the
     * surface it landed on came back opaque and a rounded corner would show the clear colour rather than the
     * desktop behind it. An element that draws no outline of its own leaves this alone.
     *
     * @param InCornerRadius The radius to round by, in pixels.
     */
    virtual void SetOuterCornerRadius(float InCornerRadius);

    /**
     * @brief Gets how far the element holds its content below its own top edge, which a submenu is placed
     * by so that its first row lines up with the row that opened it rather than its outline doing.
     *
     * @return The inset in pixels, which is zero for an element that holds its content flush.
     */
    virtual int32 GetContentTopInset() const;

    /**
     * @brief Adds all parent elements to an element path.
     *
     * @param OutParentElements The path to populate, from the window down to this element, which ends up last.
     */
    virtual void FindParentElements(FElementPath& OutParentElements);

    /**
     * @brief Adds all child elements under a specified point to the element path.
     *
     * @param ClientPosition The position to check, in the client space the elements were arranged in.
     * @param OutChildElements The element path to populate with child elements.
     */
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements);

    /**
     * @brief Recomputes and caches the desired size of this element and every descendant. Run this before
     * Tick, because a container sizes its slots from the cached child sizes.
     *
     * An element whose children are sized from something outside the tree overrides this to read that
     * something first, since by the time ComputeDesiredSize runs the children have already been measured.
     * @return The desired size of this element.
     */
    virtual IntVector2 PrepareDesiredSize();

    /**
     * @brief Marks the cached desired size of this element and every parent above it as out of date, so that
     * the next PrepareDesiredSize measures them again.
     *
     * Anything that changes what ComputeDesiredSize would return has to call this, including an element whose
     * size follows an animation or data outside the tree, which calls it for as long as that size keeps moving.
     * A clean element stops PrepareDesiredSize from walking its children, so a mutation that forgets this stays
     * wrong until something else dirties the same branch.
     */
    void InvalidateDesiredSize();

    /**
     * @brief Checks whether this element is waiting to be measured again.
     *
     * @return True when the next PrepareDesiredSize will recompute the desired size.
     */
    NODISCARD bool IsDesiredSizeDirty() const
    {
        return bDesiredSizeDirty;
    }

    /**
     * @brief Marks what this element and every parent above it draw as out of date, so the next Draw records
     * them again instead of replaying what they recorded before.
     *
     * Anything that changes what OnDraw would append has to call this, including changes that leave the
     * measured size alone: a hover, a selection, a scroll offset, a tint. InvalidateDesiredSize dirties
     * paint too, since anything that resizes an element also changes what it draws.
     */
    void InvalidatePaint();

    /**
     * @brief Checks whether this element has to be recorded again rather than replayed.
     *
     * @return True when the next Draw will walk the element instead of replaying a cached recording.
     */
    NODISCARD bool IsPaintDirty() const
    {
        return bPaintDirty;
    }

    /**
     * @brief Called from inside OnDraw by an element mid-animation, to say it has to be drawn again next
     * frame even though nothing mutated it, which also stops the enclosing subtree from being cached.
     */
    void RequestContinuousPaint() const;

    /**
     * @brief Sets whether this element may keep and replay the commands its subtree records.
     *
     * @param InPolicy Never for an element whose look changes without the tree being told, Always to cache
     * as soon as it is clean, Auto to leave it to the heuristic.
     */
    void SetDrawCachePolicy(EDrawCachePolicy InPolicy);

    /** @return Whether this element may keep and replay the commands its subtree records. */
    NODISCARD EDrawCachePolicy GetDrawCachePolicy() const
    {
        return DrawCachePolicy;
    }

    /** @brief Drops whatever this element has cached, so the next Draw records it again. */
    void ReleaseDrawCache();

    /** @return True while this element holds a recording it could replay. */
    NODISCARD bool HasDrawCache() const;

    /**
     * @brief The size cached by the last PrepareDesiredSize call.
     *
     * @return The cached desired size in pixels.
     */
    NODISCARD IntVector2 GetCachedDesiredSize() const
    {
        return CachedDesiredSize;
    }

    /**
     * @brief Checks if the element is visible.
     * 
     * @return True if the element is visible; false otherwise.
     */
    bool IsVisible() const
    { 
        return (Visibility & EVisibility::Visible) != EVisibility::None; 
    }

    /**
     * @brief Gets the visibility state of the element.
     * 
     * @return The visibility state.
     */
    EVisibility GetVisibility() const
    {
        return Visibility;
    }

    /**
     * @brief Sets the visibility state of the element.
     * 
     * @param InVisibility The new visibility state.
     */
    void SetVisibility(EVisibility InVisibility);

    /**
     * @brief Sets the parent element.
     * 
     * @param InParentElement A weak pointer to the parent element.
     */
    void SetParentElement(const TWeakPtr<FVisualElement>& InParentElement);

    /**
     * @brief Sets the content rectangle of the element.
     * 
     * @param InContentRectangle The new content rectangle.
     */
    void SetContentRectangle(const FRectangle& InContentRectangle);

    /**
     * @brief Gets the parent element.
     * 
     * @return A weak pointer to the parent element.
     */
    TWeakPtr<FVisualElement> GetParentElement() const
    {
        return ParentElement;
    }

    /** @return This element as a scroll box, or null when it is not one. */
    NODISCARD virtual FScrollBox* AsScrollBox()
    {
        return nullptr;
    }

    /**
     * @brief Gets the content rectangle of the element.
     * 
     * @return A constant reference to the content rectangle.
     */
    const FRectangle& GetContentRectangle() const
    {
        return ContentRectangle;
    }

    /**
     * @brief Gets the element activation policy.
     *
     * @return The current activation policy.
     */
    EElementActivationPolicy GetActivationPolicy() const
    {
        return ActivationPolicy;
    }

    /**
     * @brief Sets the element activation policy.
     *
     * @param InActivationPolicy The new activation policy.
     */
    void SetActivationPolicy(EElementActivationPolicy InActivationPolicy)
    {
        ActivationPolicy = InActivationPolicy;
    }

private:
    NODISCARD bool ShouldUseDrawCache() const;
    void NoteWalked(int32 CommandCount) const;

    EVisibility                         Visibility;
    EElementActivationPolicy            ActivationPolicy;
    FRectangle                          ContentRectangle;
    IntVector2                          CachedDesiredSize;
    TWeakPtr<FVisualElement>            ParentElement;
    mutable TUniquePtr<FDrawCacheBlock> DrawCacheBlock;
    EDrawCachePolicy                    DrawCachePolicy;
    mutable uint16                      LastRecordedCommandCount;
    mutable bool                        bDrawCacheBlocked;
    mutable uint8                       CleanPaintFrameCount;
    mutable uint8                       RecentDirtyFrameCount;
    bool                                bDesiredSizeDirty : 1;
    mutable bool                        bPaintDirty       : 1;
};
