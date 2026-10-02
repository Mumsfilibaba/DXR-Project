#pragma once
#include "Application/Events.h"

struct FInputHandler
{
    virtual ~FInputHandler() = default;

    /**
     * @brief Handle AnalogEvent event, if the event-handler consumes the event, return true
     * @param AnalogEvent Data for the controller event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnAnalogGamepadChange(const FAnalogGamepadEvent& /*AnalogEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle KeyEvent event, if the event-handler consumes the event, return true
     * @param KeyEvent Data for the key event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyDown(const FKeyEvent& /*KeyEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle KeyEvent event, if the event-handler consumes the event, return true
     * @param KeyEvent Data for the key event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyUp(const FKeyEvent& /*KeyEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle Key typed (String-Character), if the event-handler consumes the event, return true
     * @param KeyTypedEvent Data for the key-typed event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyChar(const FKeyEvent& /*KeyTypedEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle mouse move event, if the event-handler consumes the event, return true
     * @param CursorEvent Data for the mouse event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseMove(const FCursorEvent& /*CursorEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle mouse button event, if the event-handler consumes the event, return true
     * @param CursorEvent Data for the mouse event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseButtonDown(const FCursorEvent& /*CursorEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle mouse button event, if the event-handler consumes the event, return true
     * @param CursorEvent Data for the mouse event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseButtonUp(const FCursorEvent& /*CursorEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle mouse double-click event, if the event-handler consumes the event, return true. A handler that
     * does not override this sees the double-click as a second button press.
     * @param CursorEvent Data for the mouse event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseDoubleClick(const FCursorEvent& CursorEvent)
    {
        return OnMouseButtonDown(CursorEvent);
    }

    /**
     * @brief Handle mouse scrolled event, if the event-handler consumes the event, return true 
     * @param CursorEvent Data for the mouse event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseScrolled(const FCursorEvent& /*CursorEvent*/)
    {
        return false;
    }

    /**
     * @brief Handle a high-precision mouse event, if the event-handler consumes the event, return true
     * @param CursorEvent Data for the mouse event
     * @return Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnHighPrecisionMouseInput(const FCursorEvent& /*CursorEvent*/)
    {
        return false;
    }
};
