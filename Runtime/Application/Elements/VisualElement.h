#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"
#include "Application/Layout/LayoutTypes.h"
#include "CoreApplication/PlatformInterface/IPlatformCursor.h"

class FElementPath;
class FDrawCommandList;
struct FDrawGeometry;

/** @brief Enumeration for element visibility states. */
enum class EVisibility
{
    None    = 0,      /** @brief No visibility flags set. */
    Hidden  = BIT(1), /** @brief Element is hidden. */
    Visible = BIT(2), /** @brief Element is visible. */
};

ENUM_CLASS_OPERATORS(EVisibility);

/** @brief Policy controlling whether an element should automatically receive focus when its owning window becomes active. */
enum class EElementActivationPolicy
{
    /** @brief When the owning window is activated, focus the window content element. */
    AutoFocusOnWindowActivate,

    /** @brief Do not automatically focus this element when the owning window is activated. */
    DoNotAutoFocusOnWindowActivate,
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
     * @return True if the element is an FWindowElement; false otherwise.
     */
    virtual bool IsWindow() const;

    /**
     * @brief Whether this element takes every mouse event in its window while it is up.
     *
     * @return True while the element is modal over its window.
     */
    virtual bool CapturesAllInput() const;

    /** @brief Whether a click on this element should hand it the keyboard. The default is false. */
    virtual bool SupportsKeyboardFocus() const;

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
     * @param AllottedGeometry The rectangle and scale the element was arranged into.
     * @param OutCommandList   The list to append to.
     * @param LayerId          The layer this element draws on.
     * @return The highest layer this element or any descendant drew on.
     */
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const;

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
     * @return The desired size of this element.
     */
    IntVector2 PrepareDesiredSize();

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
    EVisibility              Visibility;
    EElementActivationPolicy ActivationPolicy;
    FRectangle               ContentRectangle;
    IntVector2               CachedDesiredSize;
    TWeakPtr<FVisualElement> ParentElement;
};
