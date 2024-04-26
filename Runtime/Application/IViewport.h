#pragma once
#include "Application/Events.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"

class FViewport;

struct IViewport
{
    virtual ~IViewport() = default;

    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) = 0;
    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnMouseMove(const FCursorEvent& MouseEvent) = 0;
    virtual FResponse OnMouseButtonDown(const FCursorEvent& MouseEvent) = 0;
    virtual FResponse OnMouseButtonUp(const FCursorEvent& MouseEvent) = 0;
    virtual FResponse OnMouseScroll(const FCursorEvent& MouseEvent) = 0;
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& MouseEvent) = 0;
	virtual FResponse OnFocusLost() = 0;
    virtual FResponse OnFocusGained() = 0;
    virtual FResponse OnMouseLeft() = 0;
    virtual FResponse OnMouseEntered() = 0;

    virtual void SetViewport(const TSharedPtr<FViewport>& InViewport) = 0;
    virtual TSharedPtr<FViewport> GetViewport() = 0;
    virtual TSharedPtr<const FViewport> GetViewport() const = 0;
};