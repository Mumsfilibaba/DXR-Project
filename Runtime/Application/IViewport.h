#pragma once
#include "Application/Events.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

class FViewport;
class FRHIViewport;

/**
 * @brief Interface for handling viewport-related events and interactions.
 */

struct IViewport
{
    /**
     * @brief Virtual destructor for the IViewport interface.
     */
    virtual ~IViewport() = default;

    /**
     * @brief Handles analog gamepad input changes.
     * 
     * Triggered when the viewport has focus, and an analog stick or trigger changes state 
     * (e.g., pressed or released more or less).
     * 
     * @param AnalogGamepadEvent The analog gamepad event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) = 0;

    /**
     * @brief Handles key or gamepad button down events.
     * 
     * Triggered when the viewport has focus, and a key or button is pressed. Also called
     * repeatedly for held-down buttons (repeat events).
     * 
     * @param KeyEvent The key event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;

    /**
     * @brief Handles key or gamepad button up events.
     * 
     * Triggered when the viewport has focus, and a key or button is released.
     * 
     * @param KeyEvent The key event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) = 0;

    /**
     * @brief Handles character input events.
     * 
     * Triggered when the viewport has focus, and a key press generates a character, considering
     * any active modifier keys.
     * 
     * @param KeyEvent The key event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;

    /**
     * @brief Handles mouse movement events.
     * 
     * Triggered whenever the mouse moves over the widget containing this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles mouse button down events.
     * 
     * Triggered when a mouse button is pressed while the cursor is over the widget containing
     * this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles mouse button up events.
     * 
     * Triggered when a mouse button is released while the cursor is over the widget containing
     * this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles mouse scroll events.
     * 
     * Triggered when the mouse wheel is scrolled (horizontally or vertically) while the cursor is
     * over the widget containing this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles mouse double-click events.
     * 
     * Triggered when a mouse button is double-clicked while the cursor is over the widget containing
     * this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles mouse leave events.
     * 
     * Triggered when the mouse leaves the widget containing this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles mouse enter events.
     * 
     * Triggered when the mouse enters the widget containing this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles high-precision mouse input events.
     * 
     * Triggered when the mouse moves with high precision over the widget containing this viewport instance.
     * 
     * @param CursorEvent The cursor event data.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) = 0;

    /**
     * @brief Handles focus lost events.
     * 
     * Triggered when the widget containing this viewport instance loses input focus.
     * 
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnFocusLost() = 0;

    /**
     * @brief Handles focus gained events.
     * 
     * Triggered when the widget containing this viewport instance gains input focus.
     * 
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnFocusGained() = 0;

    /**
     * @brief Retrieves the RHI viewport associated with this instance.
     * 
     * @return A shared reference to the RHI viewport.
     */
    virtual TSharedRef<FRHIViewport> GetViewportRHI() const = 0;

    /**
     * @brief Sets the viewport widget associated with this instance.
     * 
     * @param InViewport A shared pointer to the viewport widget to associate.
     */
    virtual void SetViewportWidget(const TSharedPtr<FViewport>& InViewport) = 0;

    /**
     * @brief Gets the viewport widget associated with this instance.
     * 
     * @return A shared pointer to the viewport widget.
     */
    virtual TSharedPtr<FViewport> GetViewportWidget() = 0;

    /**
     * @brief Gets the viewport widget associated with this instance (const version).
     * 
     * @return A shared pointer to the viewport widget.
     */
    virtual TSharedPtr<const FViewport> GetViewportWidget() const = 0;
};
