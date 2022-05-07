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
// CWindowMessageHandler

class CWindowMessageHandler
{
public:

    virtual ~CWindowMessageHandler() = default;

    virtual bool OnWindowResized(const SWindowResizeEvent& ResizeEvent)
    {
        return false;
    }

    virtual bool OnWindowMoved(const SWindowMovedEvent& WindowMovedEvent)
    {
        return false;
    }

    virtual bool OnWindowFocusChanged(const SWindowFocusChangedEvent& FocusEvent)
    {
        return false;
    }

    virtual bool OnWindowFrameMouseEvent(const SWindowFrameMouseEvent& MouseEnteredOrLeftEvent)
    {
        return false;
    }

    virtual bool OnWindowClosed(const SWindowClosedEvent& Window)
    {
        return false;
    }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
