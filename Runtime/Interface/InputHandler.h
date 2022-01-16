#pragma once
#include "Events.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Class for handling input before the game does */
class CInputHandler
{
public:

    virtual ~CInputHandler() = default;

    /* Handle KeyEvent event, if the event-handler consumes the event, return true */
    virtual bool HandleKeyEvent(const SKeyEvent& KeyEvent)
    {
        return false;
    }

    /* Handle Key typed (String-Character), if the event-handler consumes the event, return true */
    virtual bool HandleKeyTyped(SKeyTypedEvent KeyTypedEvent)
    {
        return false;
    }

    /* Handle mouse move event, if the event-handler consumes the event, return true */
    virtual bool HandleMouseMove(const SMouseMovedEvent& MouseEvent)
    {
        return false;
    }

    /* Handle mouse button event, if the event-handler consumes the event, return true */
    virtual bool HandleMouseButtonEvent(const SMouseButtonEvent& MouseEvent)
    {
        return false;
    }

    /* Handle mouse scrolled event, if the event-handler consumes the event, return true */
    virtual bool HandleMouseScrolled(const SMouseScrolledEvent& MouseEvent)
    {
        return false;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
