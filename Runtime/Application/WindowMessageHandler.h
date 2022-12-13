#pragma once
#include "Events.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

// TODO: Will be moved to the viewport in the future

struct FWindowMessageHandler
{
    virtual ~FWindowMessageHandler() = default;

    virtual bool OnWindowResized(const FWindowResizeEvent& ResizeEvent)
    {
        return false;
    }

    virtual bool OnWindowMoved(const FWindowMovedEvent& WindowMovedEvent)
    {
        return false;
    }

    virtual bool OnWindowFocusChanged(const FWindowFocusChangedEvent& FocusEvent)
    {
        return false;
    }

    virtual bool OnWindowFrameMouseEvent(const FWindowFrameMouseEvent& MouseEnteredOrLeftEvent)
    {
        return false;
    }

    virtual bool OnWindowClosed(const FWindowClosedEvent& Window)
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
