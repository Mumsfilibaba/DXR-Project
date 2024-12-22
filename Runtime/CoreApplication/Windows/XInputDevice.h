#pragma once
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/InputDevice.h"

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
    #include <XInput.h>
    #pragma comment(lib,"xinput.lib")
#else
    #include <XInput.h>
    #pragma comment(lib,"xinput9_1_0.lib")
#endif

#define NUM_BUTTONS (15)

/**
 * @struct FXInputButtonState
 * @brief Tracks the state of a single XInput button, including whether it is pressed and its repeat count.
 *
 * The `bState` bit indicates whether the button is currently pressed (1) or released (0).
 * The `RepeatCount` indicates how many consecutive frames the button has been held down.
 */
struct FXInputButtonState
{
    /** @brief Indicates if the button is pressed (1) or released (0). */
    uint8 bState : 1;

    /** @brief The number of consecutive frames that the button has been pressed. */
    uint8 RepeatCount : 7;
};

/**
 * @struct FXInputGamepadState
 * @brief Represents the current state of a gamepad as reported by XInput.
 *
 * This structure holds thumbstick axes, trigger values, button states, and connection status
 * for a single XInput-based controller. 
 */
struct FXInputGamepadState
{
    /** @brief The X-axis value of the left thumbstick, ranging from -32768 to 32767. */
    int16 LeftThumbX;

    /** @brief The Y-axis value of the left thumbstick, ranging from -32768 to 32767. */
    int16 LeftThumbY;

    /** @brief The X-axis value of the right thumbstick, ranging from -32768 to 32767. */
    int16 RightThumbX;

    /** @brief The Y-axis value of the right thumbstick, ranging from -32768 to 32767. */
    int16 RightThumbY;

    /** @brief The value of the left trigger, ranging from 0 to 255. */
    uint8 LeftTrigger;

    /** @brief The value of the right trigger, ranging from 0 to 255. */
    uint8 RightTrigger;

    /** @brief Indicates whether the gamepad is currently connected. */
    bool bConnected;

    /** @brief An array representing the state of each recognized button on the controller. */
    FXInputButtonState Buttons[NUM_BUTTONS];
};

/**
 * @class FXInputDevice
 * @brief An input device implementation for XInput-based gamepads (e.g., Xbox controllers).
 *
 * FXInputDevice inherits from FInputDevice to handle gamepad input on Windows platforms via the XInput API.
 * It polls for connected controllers, updates their states, and reports changes in button presses, thumbsticks,
 * and triggers to the engine. 
 */
class FXInputDevice : public FInputDevice
{
public:
    FXInputDevice();
    virtual ~FXInputDevice() = default;

public:

    // FInputDevice Interface
    virtual void UpdateDeviceState() override final;

    virtual bool IsDeviceConnected() const override final
    {
        return bIsDeviceConnected;
    }

public:

    /**
     * @brief Checks and updates the connectivity of the XInput device(s).
     *
     * If a controller disconnects or connects between polls, this method updates the internal
     * state to reflect that. Called internally by UpdateDeviceState.
     */
    void UpdateConnectionState();

private:
    void ProcessInputState(const XINPUT_STATE& State, uint32 GamepadIndex);

    FXInputGamepadState GamepadStates[XUSER_MAX_COUNT];
    bool bIsDeviceConnected;
};
