#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/InputDevice.h"
#include <GameController/GameController.h>

#define NUM_MAX_GAMEPADS (4)

/**
 * @struct FGCButtonState
 * @brief Stores the state of a single button on a Game Controller.
 *
 * The button can either be pressed or released (bState), and RepeatCount tracks how many 
 * consecutive frames or updates the button has been held down.
 */

struct FGCButtonState
{
    /** @brief Indicates whether the button is pressed (1) or released (0). */
    uint8 bState : 1;

    /** @brief Tracks how many consecutive updates the button has been pressed. */
    uint8 RepeatCount : 7;
};

/**
 * @struct FGCGamepadState
 * @brief Encapsulates the current state of a single gamepad using Apple's GameController framework.
 *
 * Includes thumbstick and trigger values as well as an array of button states matching
 * the engine's EGamepadButtonName enumeration.
 */

struct FGCGamepadState
{
    /** @brief Left thumbstick X-axis value, typically ranging from -1.0 to 1.0. */
    float LeftThumbX;

    /** @brief Left thumbstick Y-axis value, typically ranging from -1.0 to 1.0. */
    float LeftThumbY;

    /** @brief Right thumbstick X-axis value, typically ranging from -1.0 to 1.0. */
    float RightThumbX;

    /** @brief Right thumbstick Y-axis value, typically ranging from -1.0 to 1.0. */
    float RightThumbY;

    /** @brief Left trigger value, typically in the range [0.0, 1.0]. */
    float LeftTrigger;

    /** @brief Right trigger value, typically in the range [0.0, 1.0]. */
    float RightTrigger;

    /** @brief An array of button states corresponding to the engine's EGamepadButtonName enumerators. */
    FGCButtonState Buttons[EGamepadButtonName::Count];
};

@class FGCConnectionObserver;

/**
 * @class FGCInputDevice
 * @brief A macOS input device class that interfaces with the GameController framework.
 *
 * FGCInputDevice monitors connected controllers (GCController) and updates their states each frame.
 * It inherits from FInputDevice to integrate with the engine's input system.
 */

class FGCInputDevice : public FInputDevice
{
public:

    /**
     * @brief Creates an FGCInputDevice if the GameController framework is available and returns it as a shared pointer.
     *
     * @return A shared pointer to a newly created FGCInputDevice instance, or nullptr if creation failed.
     */
    static TSharedPtr<FGCInputDevice> CreateGCInputDevice();

public:

    FGCInputDevice();
    virtual ~FGCInputDevice();

public:

    // FInputDevice Interface
    virtual void UpdateDeviceState() override final;

    virtual bool IsDeviceConnected() const override final
    {
        return bIsDeviceConnected;
    }

public:

    /**
     * @brief Handles a newly connected GCController, adding it to the list of tracked controllers.
     *
     * @param InController The GCController that was connected.
     */
    void HandleControllerConnected(GCController* InController);

    /**
     * @brief Handles the disconnection of a GCController, removing it from the list of tracked controllers.
     *
     * @param InController The GCController that was disconnected.
     */
    void HandleControllerDisconnected(GCController* InController);

private:
    void ProcessInputState(GCExtendedGamepad* InGamepad, uint32 GamepadIndex);

    FGCConnectionObserver* Observer;
    TArray<GCController*>  ConnectedGamepads;
    FGCGamepadState        GamepadStates[NUM_MAX_GAMEPADS];
    bool                   bIsDeviceConnected;
};
