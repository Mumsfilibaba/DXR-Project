#pragma once
#include "Application/Elements/CompoundElement.h"
#include "Application/Style/UIStyle.h"

class APPLICATION_API FInteractiveElement : public FCompoundElement
{
public:
    FInteractiveElement();
    virtual ~FInteractiveElement();

    // FVisualElement Interface
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnFocusLost() override;
    virtual bool SupportsKeyboardFocus() const override;
    virtual bool IsInteractive() const override;
    virtual bool GetCursor(ECursor& OutCursor) const override;

    /**
     * @brief Enables or disables the element, which stops it responding and selects the disabled fill.
     *
     * @param bInIsEnabled True to let the element respond to input.
     */
    void SetEnabled(bool bInIsEnabled);

    /** @return True while the element responds to input. */
    NODISCARD FORCEINLINE bool IsEnabled() const
    {
        return bIsEnabled;
    }

    /** @return True while the cursor is over the element, which a disabled element never is. */
    NODISCARD FORCEINLINE bool IsHovered() const
    {
        return bIsHovered;
    }

    /**
     * @brief Gets whether a press is being held on the element.
     *
     * @return True from the button going down until it comes up again or the press is cancelled, even
     * while the cursor has been dragged off the element.
     */
    NODISCARD FORCEINLINE bool IsPressed() const
    {
        return bIsPressed;
    }

    /**
     * @brief Gets the state the element is in, which is what a subclass draws from.
     *
     * @return Disabled, Pressed, Hovered or Normal, resolved in that order.
     */
    NODISCARD EInteractionState GetInteractionState() const;

protected:

    virtual bool IsPressable() const;
    virtual bool AcceptsPressFromKey(FKey Key) const;

    virtual void OnClicked();
    virtual void OnInteractionStateChanged();
    virtual void OnDragged(const FCursorEvent& CursorEvent);

    void BeginPress();
    void CancelPress();

private:
    void EndPress(const FCursorEvent& CursorEvent);
    void UpdateHoverFromCursor(const FCursorEvent& CursorEvent);

    void SetHovered(bool bInIsHovered);
    void SetPressed(bool bInIsPressed);

    bool bIsHovered;
    bool bIsPressed;
    bool bIsEnabled;
};
