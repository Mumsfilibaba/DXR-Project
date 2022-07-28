#pragma once
#include "Events.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

// TODO: Will be moved to the viewport in the future

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowMessageHandler

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

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
