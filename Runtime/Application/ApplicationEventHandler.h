#pragma once
#include "Events.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FInputPreProcessor
{
    virtual ~FInputPreProcessor() = default;

    /**
     * @brief             - Handle AnalogEvent event, if the event-handler consumes the event, return true
     * @param AnalogEvent - Data for the controller event
     * @return            - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent)
    {
        return false;
    }

    /**
     * @brief          - Handle KeyEvent event, if the event-handler consumes the event, return true
     * @param KeyEvent - Data for the key event
     * @return         - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)
    {
        return false;
    }

    /**
     * @brief          - Handle KeyEvent event, if the event-handler consumes the event, return true
     * @param KeyEvent - Data for the key event
     * @return         - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)
    {
        return false;
    }

    /**
     * @brief               - Handle Key typed (String-Character), if the event-handler consumes the event, return true
     * @param KeyTypedEvent - Data for the key-typed event
     * @return              - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnKeyChar(const FKeyEvent& KeyTypedEvent)
    {
        return false;
    }

    /**
     * @brief             - Handle mouse move event, if the event-handler consumes the event, return true
     * @param CursorEvent - Data for the mouse event
     * @return            - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseMove(const FCursorEvent& CursorEvent)
    {
        return false;
    }

    /**
     * @brief             - Handle mouse button event, if the event-handler consumes the event, return true
     * @param CursorEvent - Data for the mouse event
     * @return            - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseButtonDown(const FCursorEvent& CursorEvent)
    {
        return false;
    }

    /**
     * @brief             - Handle mouse button event, if the event-handler consumes the event, return true
     * @param CursorEvent - Data for the mouse event
     * @return            - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseButtonUp(const FCursorEvent& CursorEvent)
    {
        return false;
    }

    /**
     * @brief             - Handle mouse scrolled event, if the event-handler consumes the event, return true 
     * @param CursorEvent - Data for the mouse event
     * @return            - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnMouseScrolled(const FCursorEvent& CursorEvent)
    {
        return false;
    }

    /**
     * @brief             - Handle a high-precision mouse event, if the event-handler consumes the event, return true
     * @param CursorEvent - Data for the mouse event
     * @return            - Returns true if the event was handled and should not be sent to other input-handlers
     */
    virtual bool OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
    {
        return false;
    }
};


struct FApplicationEventHandler
{
    virtual ~FApplicationEventHandler() = default;

    /**
     * @brief                 - Handle AnalogGamepadEvent event
     * @param AnalogGamepadEvent - Data for the controller event
     * @return                - Returns a structure with response information from the event
     */
    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief          - Handle KeyEvent event
     * @param KeyEvent - Data for the key event
     * @return         - Returns a structure with response information from the event
     */
    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief          - Handle KeyEvent event
     * @param KeyEvent - Data for the key event
     * @return         - Returns a structure with response information from the event
     */
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief               - Handle Key typed (String-Character)
     * @param KeyTypedEvent - Data for the key-typed event
     * @return              - Returns a structure with response information from the event
     */
    virtual FResponse OnKeyChar(const FKeyEvent& KeyTypedEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief            - Handle mouse move event
     * @param CursorEvent - Data for the mouse event
     * @return           - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseMove(const FCursorEvent& CursorEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief            - Handle mouse button event
     * @param CursorEvent - Data for the mouse event
     * @return           - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseButtonDown(const FCursorEvent& CursorEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief            - Handle mouse button event
     * @param CursorEvent - Data for the mouse event
     * @return           - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseButtonUp(const FCursorEvent& CursorEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief            - Handle mouse scrolled event
     * @param CursorEvent - Data for the mouse event
     * @return           - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseScroll(const FCursorEvent& CursorEvent)
    {
        return FResponse::Unhandled();
    }
    
    /**
     * @brief            - Handle mouse double-clicked event
     * @param CursorEvent - Data for the mouse event
     * @return           - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window resize events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnWindowResized(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window moved events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnWindowMoved(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window focus lost events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnWindowFocusLost(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window focus gained events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnWindowFocusGained(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window mouse left events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseLeft(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window mouse entered events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnMouseEntered(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }

    /**
     * @brief             - Handle window closed events
     * @param WindowEvent - Data for the window event
     * @return            - Returns a structure with response information from the event
     */
    virtual FResponse OnWindowClosed(const FWindowEvent& WindowEvent)
    {
        return FResponse::Unhandled();
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
