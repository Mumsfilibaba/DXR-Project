#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/InputDevice.h"
#include <GameController/GameController.h>

#define NUM_MAX_GAMEPADS (4)

struct FGCButtonState
{
    /** @brief Indicates whether the button is pressed (1) or released (0). */
    uint8 bState : 1;

    /** @brief Tracks how many consecutive updates the button has been pressed. */
    uint8 RepeatCount : 7;
};

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

class FGCInputDevice : public FInputDevice
{
public:
    static TSharedPtr<FGCInputDevice> CreateGCInputDevice();

public:
    FGCInputDevice();
    virtual ~FGCInputDevice();

    // FInputDevice Interface
    virtual void UpdateDeviceState() override final;

    virtual bool IsDeviceConnected() const override final
    {
        return bIsDeviceConnected.Load();
    }

    void HandleControllerConnected(GCController* InController);
    void HandleControllerDisconnected(GCController* InController);

private:
    void ProcessInputState(GCExtendedGamepad* InGamepad, uint32 GamepadIndex);
    void ReleaseHeldButtons(uint32 GamepadIndex);

    FGCConnectionObserver*   Observer;
    GCController*            ConnectedGamepads[NUM_MAX_GAMEPADS];
    FGCGamepadState          GamepadStates[NUM_MAX_GAMEPADS];
    mutable FCriticalSection GamepadsCS;
    AtomicBool               bIsDeviceConnected;
};
