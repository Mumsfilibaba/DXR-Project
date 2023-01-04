#pragma once
#include "ApplicationImpl.h"
#include "Events.h"
#include "Core/RefCounted.h"
#include "Core/Delegates/Event.h"
#include "Core/Math/IntVector2.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FViewportInitializer
{
    FViewportInitializer() = default;

    FViewportInitializer(uint16 InWidth, uint16 InHeight)
        : Width(InWidth)
        , Hwight(InHeight)
    { }

    uint16 Width;
    uint16 Height;
};

class APPLICATION_API FViewport
    public FRefCounted
{
public:
    FViewport(const FViewportInitializer& InInitializer);
    ~FViewport() = default;

    virtual bool Create();
    virtual bool CreateRHI();

    virtual void Destroy();
    virtual void DestroyRHI();

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)  { return false; }
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)    { return false; }
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) { return false; }

    virtual bool OnCursorMove(const FMouseMovedEvent& MouseEvent)        { return false; }
    virtual bool OnCursorButtonDown(const FMouseButtonEvent& MouseEvent) { return false; }
    virtual bool OnCursorButtonUp(const FMouseButtonEvent& MouseEvent)   { return false; }
    virtual bool OnCursorScroll(const FMouseScrolledEvent& MouseEvent)   { return false; }

    virtual bool OnViewportResized(const FWindowResizeEvent& ResizeEvent);
    virtual bool OnViewportClosed();

    virtual bool OnViewportCursorEntered() { return false; }
    virtual bool OnViewportCursorLeft()    { return false; }
    virtual bool OnViewportFocusLost()     { return false; }
    virtual bool OnViewportFocusGained()   { return false; }

    virtual FIntVector2 GetSize() const { return FIntVector2{ Initializer.Width, Initializer.Height }; }

    DECLARE_EVENT(FViewportClosedEvent, FViewport*);
    FViewportClosedEvent& GetClosedEvent() const { return ClosedEvent; }

    DECLARE_EVENT(FViewportResizedEvent, FViewport*);
    FViewportResizedEvent& GetResizedEvent() const { return ResizedEvent; }

    FRHIViewportRef   GetRHI()    const { return Viewport; }
    FGenericWindowRef GetWindow() const { return Window; };

private:
    FRHIViewportRef   Viewport;
    FGenericWindowRef Window;

    FViewportInitializer Initializer;

    mutable FViewportClosedEvent  ClosedEvent;
    mutable FViewportResizedEvent ResizedEvent;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING