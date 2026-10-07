#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Events.h"
#include "Application/Layout/LayoutTypes.h"
#include "CoreApplication/PlatformInterface/IPlatformCursor.h"

class FElementPath;
class FDrawCommandList;
class FScrollBox;
class FVisualElement;
struct FDrawGeometry;
struct FDrawCacheBlock;

enum class EVisibility : uint8
{
    /** @brief No visibility flags set. */
    None = 0,
    
    /** @brief Element is hidden. */
    Hidden = BIT(1),
    
    /** @brief Element is visible. */
    Visible = BIT(2), 
};

ENUM_CLASS_OPERATORS(EVisibility);

enum class EChildVisit : uint8
{
    /** @brief Carry on with the next child. */
    Continue,

    /** @brief Stop, leaving the remaining children unvisited. */
    Stop,
};

enum class EChildOrder : uint8
{
    /** @brief The order the children are painted in, so the first child visited is the one furthest back. */
    BackToFront,

    /** @brief The reverse of the paint order, so the first child visited is the one on top, which is the hit-test order. */
    FrontToBack,
};

using FChildVisitor = TFunctionRef<EChildVisit(FVisualElement&)>;

enum class EElementFlags : uint16
{
    None = 0,

    /** @brief The next PrepareDesiredSize measures the element again. */
    DesiredSizeDirty = FLAG(0),

    /** @brief The next Arrange runs OnArrange even when the element is given the same bounds as before. */
    ArrangeDirty = FLAG(1),

    /** @brief The next Draw records the element again instead of replaying what it recorded before. */
    PaintDirty = FLAG(2),

    /** @brief A capture of the subtree was refused, so the element stops volunteering as a cache root. */
    DrawCacheBlocked = FLAG(3),

    /** @brief The element takes part in hit testing. Clearing it lets the cursor pass through to what lies below. */
    HitTestable = FLAG(4),

    /** @brief The element is an FWindow. */
    IsWindow = FLAG(5),

    /** @brief The element answers mouse input rather than only drawing. */
    IsInteractive = FLAG(6),

    /** @brief The element is an FScrollBox. */
    IsScrollBox = FLAG(7),

    /** @brief The window content is focused when its window is activated, which EElementActivationPolicy describes. */
    AutoFocusOnWindowActivate = FLAG(8),

    /**
     * @brief The element, or something below it, places children outside its own rectangle, so a hit test outside
     * the rectangle still has to look below it. Attaching such an element to a parent marks every ancestor too.
     */
    HitTestOverflow = FLAG(9),
};

ENUM_CLASS_OPERATORS(EElementFlags);

enum class EElementActivationPolicy : uint8
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

    /**
     * @brief Stores the assigned bounds as the content rectangle and then arranges the children inside it. The
     * arrange is skipped when the bounds are the ones the element already has and nothing marked it arrange-dirty
     * since, which is what lets a clean subtree cost one comparison per frame.
     *
     * @param AssignedBounds The rectangle the parent gives the element.
     */
    void Arrange(const FRectangle& AssignedBounds);

    /**
     * @brief Marks this element and every parent above it to be arranged again even if the bounds they are given do
     * not change, which is what a setter calls when it moves children without changing any desired size: a scroll
     * offset, a splitter fraction, a pan or zoom. InvalidateDesiredSize does this too.
     */
    void InvalidateArrange();

    /**
     * @brief Called from inside OnArrange by an element mid-animation or following something outside the tree, to
     * say it has to be arranged again next frame even though nothing mutated it.
     */
    void RequestContinuousArrange() const;

    /** @return True when the next Arrange runs OnArrange whatever bounds it is given. */
    NODISCARD FORCEINLINE bool IsArrangeDirty() const
    {
        return HasAnyElementFlags(EElementFlags::ArrangeDirty);
    }

    /**
     * @brief Checks if the element is a window.
     *
     * @return True if the element is an FWindow, false otherwise.
     */
    NODISCARD FORCEINLINE bool IsWindow() const
    {
        return HasAnyElementFlags(EElementFlags::IsWindow);
    }

    /**
     * @brief Whether the element answers mouse input rather than only drawing.
     *
     * A title bar asks this of its descendants to work out which parts of the caption stay clickable
     * instead of dragging the window.
     * @return True for an FInteractiveElement or anything else that marked itself interactive.
     */
    NODISCARD FORCEINLINE bool IsInteractive() const
    {
        return HasAnyElementFlags(EElementFlags::IsInteractive);
    }

    /** @return This element as a scroll box, or null when it is not one. */
    NODISCARD FScrollBox* AsScrollBox();

    /** @return True while the element takes part in hit testing. */
    NODISCARD FORCEINLINE bool IsHitTestable() const
    {
        return HasAnyElementFlags(EElementFlags::HitTestable);
    }

    /**
     * @brief Sets whether the element takes part in hit testing. An element that does not is skipped together
     * with its whole subtree, so the cursor reaches whatever lies below it.
     *
     * @param bInHitTestable True to let the cursor find the element.
     */
    void SetHitTestable(bool bInHitTestable);

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
     * @brief Calls a visitor with every direct child of this element, without collecting them anywhere first.
     *
     * @param Visitor Called once per child, and able to stop the walk early.
     * @param Order   The order to walk the children in.
     * @return Stop when the visitor stopped the walk, Continue when it saw every child.
     */
    FORCEINLINE EChildVisit ForEachChild(FChildVisitor Visitor, EChildOrder Order = EChildOrder::BackToFront) const
    {
        return VisitChildren(Visitor, Order);
    }

    /**
     * @brief Appends the direct children of this element in paint order, back to front.
     *
     * @param OutChildren The array to append to.
     */
    void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const;

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
    void FindParentElements(FElementPath& OutParentElements);

    /**
     * @brief Appends this element and the chain of descendants under a point to a path, ordered from this element
     * down to the one on top. A hidden or non hit-testable element, or one the point falls outside of, adds
     * nothing and is not descended into, so the cost follows the depth of the tree rather than its size.
     *
     * @param ClientPosition The position to check, in the client space the elements were arranged in.
     * @param OutPath        The path to append to.
     * @return True when the element took the point and was added to the path.
     */
    bool HitTest(const IntVector2& ClientPosition, FElementPath& OutPath);

    /**
     * @brief A counter that moves whenever anything that could change a hit test result changes: an arranged
     * rectangle, a visibility or hit-testability flag, a parent, or any state an element repaints for. A path
     * found at one value stays valid for as long as the counter holds it.
     *
     * @return The current value.
     */
    NODISCARD static uint64 GetHitTestGeneration();

    /**
     * @brief Recomputes and caches the desired size of this element and every descendant. Run this before
     * Arrange, because a container sizes its slots from the cached child sizes.
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
        return HasAnyElementFlags(EElementFlags::DesiredSizeDirty);
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
        return HasAnyElementFlags(EElementFlags::PaintDirty);
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
        return HasAnyElementFlags(EElementFlags::AutoFocusOnWindowActivate)
            ? EElementActivationPolicy::AutoFocusOnWindowActivate
            : EElementActivationPolicy::DoNotAutoFocusOnWindowActivate;
    }

    /**
     * @brief Sets the element activation policy.
     *
     * @param InActivationPolicy The new activation policy.
     */
    void SetActivationPolicy(EElementActivationPolicy InActivationPolicy)
    {
        if (InActivationPolicy == EElementActivationPolicy::AutoFocusOnWindowActivate)
        {
            SetElementFlags(EElementFlags::AutoFocusOnWindowActivate);
        }
        else
        {
            ClearElementFlags(EElementFlags::AutoFocusOnWindowActivate);
        }
    }

protected:

    virtual EChildVisit VisitChildren(FChildVisitor& Visitor, EChildOrder Order) const;
    virtual void HitTestChildren(const IntVector2& ClientPosition, FElementPath& OutPath);

    FORCEINLINE void AddElementFlags(EElementFlags InFlags)
    {
        Flags |= InFlags;
    }

    void EnableHitTestOverflow();

    /** @return True when this element, or an element below it, can be hit outside its content rectangle. */
    NODISCARD FORCEINLINE bool HasHitTestOverflow() const
    {
        return HasAnyElementFlags(EElementFlags::HitTestOverflow);
    }

    template<typename ElementType>
    NODISCARD static FORCEINLINE EChildVisit VisitChild(FChildVisitor& Visitor, const TSharedPtr<ElementType>& Child)
    {
        return Child ? Visitor(*Child) : EChildVisit::Continue;
    }

    template<typename... ElementTypes>
    static EChildVisit VisitChildList(FChildVisitor& Visitor, EChildOrder Order, const TSharedPtr<ElementTypes>&... Children)
    {
        FVisualElement* const List[] = { static_cast<FVisualElement*>(Children.Get())... };

        constexpr int32 NumChildren = static_cast<int32>(sizeof...(ElementTypes));
        for (int32 Step = 0; Step < NumChildren; ++Step)
        {
            FVisualElement* Child = List[Order == EChildOrder::BackToFront ? Step : (NumChildren - 1 - Step)];
            if (Child && Visitor(*Child) == EChildVisit::Stop)
            {
                return EChildVisit::Stop;
            }
        }

        return EChildVisit::Continue;
    }

    template<typename ArrayType, typename ProjectionType>
    static EChildVisit VisitChildArray(FChildVisitor& Visitor, EChildOrder Order, const ArrayType& Items, ProjectionType&& Projection)
    {
        const int32 NumItems = static_cast<int32>(Items.Size());
        for (int32 Step = 0; Step < NumItems; ++Step)
        {
            const int32 Index = Order == EChildOrder::BackToFront ? Step : (NumItems - 1 - Step);
            if (VisitChild(Visitor, Projection(Items[Index])) == EChildVisit::Stop)
            {
                return EChildVisit::Stop;
            }
        }

        return EChildVisit::Continue;
    }

private:
    NODISCARD FORCEINLINE FVisualElement* GetLiveParent() const
    {
        return ParentElement.IsValid() ? ParentElement.Get() : nullptr;
    }

    NODISCARD FORCEINLINE bool HasAnyElementFlags(EElementFlags InFlags) const
    {
        return (Flags & InFlags) != EElementFlags::None;
    }

    FORCEINLINE void SetElementFlags(EElementFlags InFlags) const
    {
        Flags |= InFlags;
    }

    FORCEINLINE void ClearElementFlags(EElementFlags InFlags) const
    {
        Flags &= ~InFlags;
    }

    NODISCARD bool ShouldUseDrawCache() const;
    void NoteWalked(int32 CommandCount) const;

    void PropagateHitTestOverflow();

    FRectangle                          ContentRectangle;
    IntVector2                          CachedDesiredSize;
    TWeakPtr<FVisualElement>            ParentElement;
    mutable TUniquePtr<FDrawCacheBlock> DrawCacheBlock;
    mutable uint16                      LastRecordedCommandCount;
    mutable EElementFlags               Flags;
    EVisibility                         Visibility;
    EDrawCachePolicy                    DrawCachePolicy;
    mutable uint8                       CleanPaintFrameCount;
    mutable uint8                       RecentDirtyFrameCount;
};

static_assert(sizeof(void*) != 8 || sizeof(FVisualElement) == 80, "Every element pays for FVisualElement, so a new member has to fit its padding or be justified");
