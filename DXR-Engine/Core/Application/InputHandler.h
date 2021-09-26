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

    /* Handle KeyEvent event, if the eventhandler handles the event, return true */
    virtual bool OnKeyEvent( const SKeyEvent& KeyEvent )
    {
        return false;
    }

    /* Handle Key typed (String-Character), if the eventhandler handles the event, return true */
    virtual bool OnKeyTyped( SKeyTypedEvent KeyTypedEvent )
    {
        return false;
    }

    /* Handle mouse move event, if the eventhandler handles the event, return true */
    virtual bool OnMouseMove( const SMouseMovedEvent& MouseEvent )
    {
        return false;
    }

    /* Handle mouse button event, if the eventhandler handles the event, return true */
    virtual bool OnMouseButtonEvent( const SMouseButtonEvent& MouseEvent )
    {
        return false;
    }

    /* Handle mouse scrolled event, if the eventhandler handles the event, return true */
    virtual bool OnMouseScrolled( const SMouseScrolledEvent& MouseEvent )
    {
        return false;
    }

    /* Return the priority of this inputhandler, higher will be processed first */
    virtual uint32 GetPriority() const
    {
        return 0;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
