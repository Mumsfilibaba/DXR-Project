#pragma once
#include "Events.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FInputHandler

struct FInputHandler
{
    virtual ~FInputHandler() = default;

    /**
     * @brief: Handle KeyEvent event, if the event-handler consumes the event, return true
     * 
     * @param KeyEvent: Data for the key event
     * @return: Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool HandleKeyEvent(const FKeyEvent& KeyEvent)
    {
        return false;
    }

    /**
     * @brief: Handle Key typed (String-Character), if the event-handler consumes the event, return true
     *
     * @param KeyTypedEvent: Data for the key-typed event
     * @return: Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool HandleKeyTyped(FKeyCharEvent KeyTypedEvent)
    {
        return false;
    }

    /**
     * @brief: Handle mouse move event, if the event-handler consumes the event, return true 
     *
     * @param MouseEvent: Data for the mouse event
     * @return: Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool HandleMouseMove(const FMouseMovedEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief: Handle mouse button event, if the event-handler consumes the event, return true 
     *
     * @param MouseEvent: Data for the mouse event
     * @return: Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool HandleMouseButtonEvent(const FMouseButtonEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief: Handle mouse scrolled event, if the event-handler consumes the event, return true 
     *
     * @param MouseEvent: Data for the mouse event
     * @return: Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool HandleMouseScrolled(const FMouseScrolledEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief: Handle a high-precision mouse event, if the event-handler consumes the event, return true 
     *
     * @param HighPrecisionMouseEvent: Data for the mouse event
     * @return: Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool HandleHighPrecisionMouseInput(const FHighPrecisionMouseEvent& HighPrecisionMouseEvent)
    {
        return false;
    }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
