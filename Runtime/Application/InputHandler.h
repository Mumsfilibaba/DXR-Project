#pragma once
#include "Application/Core/Events.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FInputHandler
{
    virtual ~FInputHandler() = default;

    /**
     * @brief                 - Handle ControllerEvent event, if the event-handler consumes the event, return true
     * @param ControllerEvent - Data for the controller event
     * @return                - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnControllerButtonDownEvent(const FControllerEvent& ControllerEvent)
    {
        return false;
    }

    /**
     * @brief                 - Handle ControllerEvent event, if the event-handler consumes the event, return true
     * @param ControllerEvent - Data for the controller event
     * @return                - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnControllerButtonUpEvent(const FControllerEvent& ControllerEvent)
    {
        return false;
    }
    
    /**
     * @brief                 - Handle ControllerEvent event, if the event-handler consumes the event, return true
     * @param ControllerEvent - Data for the controller event
     * @return                - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnControllerAnalogEvent(const FControllerEvent& ControllerEvent)
    {
        return false;
    }

    /**
     * @brief          - Handle KeyEvent event, if the event-handler consumes the event, return true
     * @param KeyEvent - Data for the key event
     * @return         - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyDownEvent(const FKeyEvent& KeyEvent)
    {
        return false;
    }

    /**
     * @brief          - Handle KeyEvent event, if the event-handler consumes the event, return true
     * @param KeyEvent - Data for the key event
     * @return         - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyUpEvent(const FKeyEvent& KeyEvent)
    {
        return false;
    }

    /**
     * @brief               - Handle Key typed (String-Character), if the event-handler consumes the event, return true
     * @param KeyTypedEvent - Data for the key-typed event
     * @return              - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyCharEvent(const FKeyEvent& KeyTypedEvent)
    {
        return false;
    }

    /**
     * @brief            - Handle mouse move event, if the event-handler consumes the event, return true
     * @param MouseEvent - Data for the mouse event
     * @return           - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseMove(const FMouseEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief            - Handle mouse button event, if the event-handler consumes the event, return true
     * @param MouseEvent - Data for the mouse event
     * @return           - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseButtonDownEvent(const FMouseEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief            - Handle mouse button event, if the event-handler consumes the event, return true
     * @param MouseEvent - Data for the mouse event
     * @return           - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseButtonUpEvent(const FMouseEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief            - Handle mouse scrolled event, if the event-handler consumes the event, return true 
     * @param MouseEvent - Data for the mouse event
     * @return           - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseScrolled(const FMouseEvent& MouseEvent)
    {
        return false;
    }

    /**
     * @brief            - Handle a high-precision mouse event, if the event-handler consumes the event, return true
     * @param MouseEvent - Data for the mouse event
     * @return           - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnHighPrecisionMouseInput(const FMouseEvent& MouseEvent)
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
