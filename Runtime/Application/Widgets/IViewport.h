#pragma once
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Core/Events.h"

class FViewportWidget;

struct IViewport
{
    virtual ~IViewport() = default;

    virtual FResponse OnControllerButtonDown  (const FControllerEvent& ControllerEvent) = 0;
    virtual FResponse OnControllerButtonUp    (const FControllerEvent& ControllerEvent) = 0;
    virtual FResponse OnControllerButtonAnalog(const FControllerEvent& ControllerEvent) = 0;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyUp  (const FKeyEvent& KeyEvent) = 0;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) = 0;

    virtual FResponse OnMouseMove       (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseButtonDown (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseButtonUp   (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseEntered    (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseScroll     (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseLeft       (const FMouseEvent& MouseEvent) = 0;
    virtual FResponse OnMouseDoubleClick(const FMouseEvent& MouseEvent) = 0;

    virtual void SetViewportWidget(const TSharedPtr<FViewportWidget>& InViewport) = 0;

    virtual TSharedPtr<FViewportWidget>       GetViewportWidget()       = 0;
    virtual TSharedPtr<const FViewportWidget> GetViewportWidget() const = 0;
};