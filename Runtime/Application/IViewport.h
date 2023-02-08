#pragma once
#include "Events.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHIResources.h"

class FViewport;

struct IViewport
{
    virtual ~IViewport() = default;

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)  = 0;
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)    = 0;    
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) = 0;

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent)      = 0;
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent)     = 0;
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent)       = 0;
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent) = 0;
    virtual bool OnMouseEntered() = 0;
    virtual bool OnMouseLeft()    = 0;

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent) = 0;
    virtual bool OnWindowMove(const FWindowMovedEvent& InMoveEvent)        = 0;
    virtual bool OnWindowFocusGained() = 0;
    virtual bool OnWindowFocusLost()   = 0;
    virtual bool OnWindowClosed()      = 0;

    virtual TSharedPtr<FViewport>       GetViewport()       = 0;
    virtual TSharedPtr<const FViewport> GetViewport() const = 0;
};