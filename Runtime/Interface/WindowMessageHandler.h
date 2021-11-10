#pragma once
#include "Events.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Handles events regarding windows, and can process them before it gets to the viewport */
class CWindowMessageHandler
{
public:

    /* Handle Window Resized event, if the handler handles the event, return true */
    virtual bool OnWindowResized( const SWindowResizeEvent& ResizeEvent )
    {
        return false;
    }

    /* Handle Window Moved  event, if the handler handles the event, return true */
    virtual bool OnWindowMoved( const SWindowMovedEvent& WindowMovedEvent )
    {
        return false;
    }

    /* Handle Window Focus changed event, if the handler handles the event, return true */
    virtual bool OnWindowFocusChanged( const SWindowFocusChangedEvent& FocusEvent )
    {
        return false;
    }

    /* Handle Mouse entered or exited the window frame- event, if the handler handles the event, return true */
    virtual bool OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& MouseEnteredOrLeftEvent )
    {
        return false;
    }

    /* Handle Window closed event, if the handler handles the event, return true */
    virtual bool OnWindowClosed( const SWindowClosedEvent& Window )
    {
        return false;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
