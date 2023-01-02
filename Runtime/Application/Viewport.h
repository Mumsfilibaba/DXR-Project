#pragma once
#include "ApplicationImpl.h"
#include "Events.h"
#include "Core/RefCounted.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class APPLICATION_API FViewport
    public FRefCounted
{
public:
    FViewport();
    ~FViewport() = default;

    virtual bool Initialize();

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)  { return false; }
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)    { return false; }
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) { return false; }

    virtual bool OnCursorMove(const FMouseMovedEvent& MouseEvent)        { return false; }
    virtual bool OnCursorButtonDown(const FMouseButtonEvent& MouseEvent) { return false; }
    virtual bool OnCursorButtonUp(const FMouseButtonEvent& MouseEvent)   { return false; }
    virtual bool OnCursorScroll(const FMouseScrolledEvent& MouseEvent)   { return false; }

    virtual bool OnViewportResized(const FWindowResizeEvent& ResizeEvent) { return false; }
    virtual bool OnViewportCursorEntered() { return false; }
    virtual bool OnViewportCursorLeft()    { return false; }
    virtual bool OnViewportFocusLost()     { return false; }
    virtual bool OnViewportFocusGained()   { return false; }
    virtual bool OnViewportClosed()        { return false; }

    DECLARE_EVENT(FViewportClosedEvent, FViewport);
    FViewportClosedEvent GetClosedEvent() const { return ClosedEvent; }

    DECLARE_EVENT(FViewportResizedEvent, FViewport);
    FViewportResizedEvent GetResizedEvent() const { return ResizedEvent; }

    FRHIViewportRef   GetRHI()    const { return Viewport; }
    FGenericWindowRef GetWindow() const { return Window; };

private:
    FViewportClosedEvent  ClosedEvent;
    FViewportResizedEvent ResizedEvent;

    FRHIViewportRef   Viewport;
    FGenericWindowRef Window;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING