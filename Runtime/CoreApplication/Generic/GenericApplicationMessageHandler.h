#pragma once
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericApplicationMisc.h"
#include "CoreApplication/Generic/InputCodes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

/**
 * @struct FGenericApplicationMessageHandler
 * @brief Interface for handling various application messages and input events.
 *
 * FGenericApplicationMessageHandler serves as an abstract base class defining a set of virtual
 * methods to handle different types of input and window events. Subclasses should override
 * these methods to implement specific behaviors in response to user interactions and system events.
 *
 * The methods return a boolean indicating whether the event was handled (true) or not (false).
 * This allows for event propagation control, where unhandled events can be passed to other handlers.
 */
struct FGenericApplicationMessageHandler
{
    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~FGenericApplicationMessageHandler() = default;

    /**
     * @brief Called when a gamepad button is released.
     * 
     * @param Button The name of the gamepad button that was released.
     * @param GamepadIndex The index of the gamepad (useful for handling multiple gamepads).
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
    {
        return false;
    }

    /**
     * @brief Called when a gamepad button is pressed.
     * 
     * @param Button The name of the gamepad button that was pressed.
     * @param GamepadIndex The index of the gamepad.
     * @param bIsRepeat Indicates if the button press is a repeated event due to holding the button down.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
    {
        return false;
    }

    /**
     * @brief Called when an analog input on the gamepad changes.
     * 
     * @param AnalogSource The name of the analog input source (e.g., thumbstick, trigger).
     * @param GamepadIndex The index of the gamepad.
     * @param AnalogValue The new value of the analog input.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
    {
        return false;
    }

    /**
     * @brief Called when a keyboard key is released.
     * 
     * @param KeyCode The code of the key that was released.
     * @param ModifierKeyState The state of modifier keys (e.g., Shift, Ctrl) at the time of the event.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModifierKeyState)
    {
        return false;
    }

    /**
     * @brief Called when a keyboard key is pressed.
     * 
     * @param KeyCode The code of the key that was pressed.
     * @param bIsRepeat Indicates if the key press is a repeated event due to holding the key down.
     * @param ModifierKeyState The state of modifier keys at the time of the event.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModifierKeyState)
    {
        return false;
    }

    /**
     * @brief Called when a character is input via the keyboard.
     * 
     * @param Character The Unicode character that was input.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnKeyChar(uint32 Character)
    {
        return false;
    }

    /**
     * @brief Called when the mouse is moved.
     * 
     * @param MouseX The new X-coordinate of the mouse cursor.
     * @param MouseY The new Y-coordinate of the mouse cursor.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseMove(int32 MouseX, int32 MouseY)
    {
        return false;
    }

    /**
     * @brief Called when a mouse button is pressed.
     * 
     * @param PlatformWindow The window in which the mouse event occurred.
     * @param Button The name of the mouse button that was pressed.
     * @param ModifierKeyState The state of modifier keys at the time of the event.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState)
    {
        return false;
    }

    /**
     * @brief Called when a mouse button is released.
     * 
     * @param Button The name of the mouse button that was released.
     * @param ModifierKeyState The state of modifier keys at the time of the event.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState)
    {
        return false;
    }

    /**
     * @brief Called when a mouse button is double-clicked.
     * 
     * @param Button The name of the mouse button that was double-clicked.
     * @param ModifierKeyState The state of modifier keys at the time of the event.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState)
    {
        return false;
    }

    /**
     * @brief Called when the mouse wheel is scrolled.
     * 
     * @param WheelDelta The amount the wheel has scrolled.
     * @param bVertical Indicates whether the scroll is vertical (true) or horizontal (false).
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical)
    {
        return false;
    }

    /**
     * @brief Called when the mouse cursor enters the application's window.
     * 
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseEntered()
    {
        return false;
    }

    /**
     * @brief Called when the mouse cursor leaves the application's window.
     * 
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMouseLeft()
    {
        return false;
    }

    /**
     * @brief Called when high-precision mouse input is received.
     * 
     * @param MouseX The X-coordinate of the mouse cursor.
     * @param MouseY The Y-coordinate of the mouse cursor.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnHighPrecisionMouseInput(int32 MouseX, int32 MouseY)
    {
        return false;
    }

    /**
     * @brief Called when a window is resized.
     * 
     * @param Window The window that was resized.
     * @param Width The new width of the window.
     * @param Height The new height of the window.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height)
    {
        return false;
    }
    
    /**
     * @brief Called when a window is in the process of being resized.
     * 
     * @param Window The window that is being resized.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnWindowResizing(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    /**
     * @brief Called when a window is moved.
     * 
     * @param Window The window that was moved.
     * @param x The new X-coordinate of the window's position.
     * @param y The new Y-coordinate of the window's position.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y)
    {
        return false;
    }

    /**
     * @brief Called when a window loses focus.
     * 
     * @param Window The window that lost focus.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    /**
     * @brief Called when a window gains focus.
     * 
     * @param Window The window that gained focus.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    /**
     * @brief Called when a window is closed.
     * 
     * @param Window The window that was closed.
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnWindowClosed(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    /**
     * @brief Called when the monitor configuration changes (e.g., display resolution changes, new monitor connected).
     * 
     * @return true if the event was handled, false otherwise.
     */
    virtual bool OnMonitorConfigurationChange()
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
