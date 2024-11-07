#pragma once
#include "Application/Events.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

class FViewport;
class FRHIViewport;

struct IViewport
{
    virtual ~IViewport() = default;

    // This event is sent when the viewport containing this IViewport-instance has focus and a analog stick or
    // trigger is changed, in other words, released or pressed down more or less.
    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) = 0;
    
    // This event is sent when the viewport containing this IViewport-instance has focus and a keyboard-key or
    // gamepad-button is pressed. This is also called when the button is held down and sends repeat-events.
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;
    
    // This event is sent when the viewport containing this IViewport-instance has focus and a keyboard-key or
    // gamepad-button is released.
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) = 0;
    
    // This event is sent when the viewport containing this IViewport-instance has focus and a keyboard-key is
    // pressed. This generates a character for the pressed key, based on any modifier-key that is also pressed.
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;
    
    // This event is sent whenever the mouse is moved over the widget that hovering this IViewport-instance
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever a mousebutton is pressed and the cursor is hovering the widget that contains
    // this IViewport-instance.
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever a mousebutton is released and the cursor is hovering the widget that contains
    // this IViewport-instance.
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever the cursor is hovering the widget that contains this IViewport-instance
    // and the mousewheel is scrolled horizontally or vertically.
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever the cursor is hovering the widget that contains this IViewport-instance
    // and the mousebutton is pressed twice within a short period of time.
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever the mouse leaves (stops hovering) the widget that contains this
    // IViewport-interface.
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever the mouse entered (starts hovering) the widget that contains this
    // IViewport-interface.
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) = 0;
    
    // This event is sent whenever the mouse is moved over widget that contains this IViewport-interface.
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) = 0;
	
    // This event is sent whenever the widget loose input-focus
    virtual FEventResponse OnFocusLost() = 0;
    
    // This event is sent whenever the widget gains input-focus
    virtual FEventResponse OnFocusGained() = 0;

    virtual TSharedRef<FRHIViewport> GetViewportRHI() const = 0;

    virtual void SetViewportWidget(const TSharedPtr<FViewport>& InViewport) = 0;

    virtual TSharedPtr<FViewport>       GetViewportWidget()       = 0;
    virtual TSharedPtr<const FViewport> GetViewportWidget() const = 0;
    
};
