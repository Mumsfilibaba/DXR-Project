#pragma once
#include "Application/Events.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"

class FViewport;

struct IViewport
{
    virtual ~IViewport() = default;

    virtual FResponse OnControllerAnalog    (const FControllerEvent& ControllerEvent) = 0;
    virtual FResponse OnControllerButtonDown(const FControllerEvent& ControllerEvent) = 0;
    virtual FResponse OnControllerButtonUp  (const FControllerEvent& ControllerEvent) = 0;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyUp  (const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;

    virtual FResponse OnMouseMove       (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseButtonDown (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseButtonUp   (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseScroll     (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseDoubleClick(const FMouseEvent& MouseEvent) = 0;

	virtual FResponse OnFocusLost   (const FWindowEvent& WindowEvent) = 0;
	virtual FResponse OnFocusGained (const FWindowEvent& WindowEvent) = 0;
	virtual FResponse OnMouseLeft   (const FWindowEvent& WindowEvent) = 0;
	virtual FResponse OnMouseEntered(const FWindowEvent& WindowEvent) = 0;

    virtual void SetViewport(const TSharedPtr<FViewport>& InViewport) = 0;

    virtual TSharedPtr<FViewport>       GetViewport()       = 0;
    virtual TSharedPtr<const FViewport> GetViewport() const = 0;
};