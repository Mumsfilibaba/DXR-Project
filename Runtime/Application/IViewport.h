#pragma once
#include "Application/Events.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

class FViewport;
class FRHIViewport;

struct IViewport
{
    virtual ~IViewport() = default;

    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) = 0;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) = 0;
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) = 0;
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) = 0;
	virtual FEventResponse OnFocusLost() = 0;
    virtual FEventResponse OnFocusGained() = 0;

    virtual TSharedRef<FRHIViewport> GetViewportRHI() const = 0;

    virtual void SetViewportWidget(const TSharedPtr<FViewport>& InViewport) = 0;

    virtual TSharedPtr<FViewport>       GetViewportWidget()       = 0;
    virtual TSharedPtr<const FViewport> GetViewportWidget() const = 0;
    
};
