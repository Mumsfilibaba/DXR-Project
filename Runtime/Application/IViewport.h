#pragma once
#include "Application/Events.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

class FViewport;
class FRHIViewport;

struct IViewport
{
    virtual ~IViewport() = default;

    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) = 0;
    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnMouseMove(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnMouseScroll(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnMouseLeft(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnMouseEntered(const FCursorEvent& CursorEvent) = 0;
    virtual FResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) = 0;
	virtual FResponse OnFocusLost() = 0;
    virtual FResponse OnFocusGained() = 0;

    virtual TSharedRef<FRHIViewport> GetViewportRHI() const = 0;

    virtual void SetViewportWidget(const TSharedPtr<FViewport>& InViewport) = 0;

    virtual TSharedPtr<FViewport>       GetViewportWidget()       = 0;
    virtual TSharedPtr<const FViewport> GetViewportWidget() const = 0;
    
};
