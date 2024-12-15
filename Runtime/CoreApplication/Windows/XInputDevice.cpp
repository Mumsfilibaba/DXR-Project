#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Windows/XInputDevice.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

static TAutoConsoleVariable<int32> CVarXInputButtonRepeatDelay(
    "XInput.ButtonRepeatDelay",
    "Number of repeated messages that gets ignored before sending repeat events",
    60,
    EConsoleVariableFlags::Default);

static FORCEINLINE float NormalizeThumbStick(int16 ThumbValue)
{
    // Max value is different for negative vs. positive input in two's complement
    const float ThumbMaxValue = (ThumbValue < 0) ? 32768.0f : 32767.0f;
    return static_cast<float>(ThumbValue) / ThumbMaxValue;
}

static FORCEINLINE float NormalizeTrigger(uint8 TriggerValue)
{
    constexpr float TriggerMaxValue = 255.0f;
    return static_cast<float>(TriggerValue) / TriggerMaxValue;
}

FXInputDevice::FXInputDevice()
    : FInputDevice()
    , bIsDeviceConnected(false)
{
    FMemory::Memzero(GamepadStates, sizeof(FXInputGamepadState) * XUSER_MAX_COUNT);
}

void FXInputDevice::UpdateDeviceState()
{
    bool bFoundDevice = false;

    // Update the state for all controllers that are already marked as connected
    for (DWORD GamepadIndex = 0; GamepadIndex < XUSER_MAX_COUNT; ++GamepadIndex)
    {
        FXInputGamepadState& CurrentState = GamepadStates[GamepadIndex];
        if (!CurrentState.bConnected)
        {
            continue;
        }

        XINPUT_STATE State;
        FMemory::Memzero(&State, sizeof(XINPUT_STATE));

        DWORD Result = XInputGetState(GamepadIndex, &State);
        if (Result == ERROR_SUCCESS)
        {
            ProcessInputState(State, GamepadIndex);
            bFoundDevice = true;
        }
        else
        {
            CurrentState.bConnected = false;
        }
    }

    // Update device connected flag
    bIsDeviceConnected = bFoundDevice;
}

void FXInputDevice::UpdateConnectionState()
{
    bool bFoundDevice = false;

    // Check all 4 possible XInput controllers
    for (DWORD UserIndex = 0; UserIndex < XUSER_MAX_COUNT; ++UserIndex)
    {
        XINPUT_STATE State;
        FMemory::Memzero(&State, sizeof(XINPUT_STATE));

        DWORD Result = XInputGetState(UserIndex, &State);
        if (Result == ERROR_SUCCESS)
        {
            GamepadStates[UserIndex].bConnected = true;
            bFoundDevice = true;
        }
        else
        {
            GamepadStates[UserIndex].bConnected = false;
        }
    }

    bIsDeviceConnected = bFoundDevice;
}

void FXInputDevice::ProcessInputState(const XINPUT_STATE& State, uint32 GamepadIndex)
{
    FXInputGamepadState& CurrentState = GamepadStates[GamepadIndex];
    const XINPUT_GAMEPAD& Gamepad = State.Gamepad;

    TSharedPtr<FGenericApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler();
    if (!CurrentMessageHandler)
    {
        return; // If there's no valid message handler, no need to process further
    }

    constexpr int32 MaxButtonRepeatDelay = (1 << 7) - 1; // 127
    int32 RepeatDelay = FMath::Clamp(CVarXInputButtonRepeatDelay.GetValue(), 0, MaxButtonRepeatDelay);

    const WORD GamepadButtons = Gamepad.wButtons;
    auto IsButtonDown = [GamepadButtons](WORD ButtonMask) -> bool
    {
        return (GamepadButtons & ButtonMask) != 0;
    };

    // Prepare a boolean array for all button states
    bool bCurrentStates[NUM_BUTTONS];
    FMemory::Memzero(bCurrentStates, sizeof(bCurrentStates));

    // D-Pad
    bCurrentStates[EGamepadButtonName::DPadUp]    = IsButtonDown(XINPUT_GAMEPAD_DPAD_UP);
    bCurrentStates[EGamepadButtonName::DPadDown]  = IsButtonDown(XINPUT_GAMEPAD_DPAD_DOWN);
    bCurrentStates[EGamepadButtonName::DPadLeft]  = IsButtonDown(XINPUT_GAMEPAD_DPAD_LEFT);
    bCurrentStates[EGamepadButtonName::DPadRight] = IsButtonDown(XINPUT_GAMEPAD_DPAD_RIGHT);

    // Face Buttons
    bCurrentStates[EGamepadButtonName::FaceUp]    = IsButtonDown(XINPUT_GAMEPAD_Y);
    bCurrentStates[EGamepadButtonName::FaceDown]  = IsButtonDown(XINPUT_GAMEPAD_A);
    bCurrentStates[EGamepadButtonName::FaceLeft]  = IsButtonDown(XINPUT_GAMEPAD_X);
    bCurrentStates[EGamepadButtonName::FaceRight] = IsButtonDown(XINPUT_GAMEPAD_B);

    // Thumbstick Clicks
    bCurrentStates[EGamepadButtonName::RightThumb] = IsButtonDown(XINPUT_GAMEPAD_RIGHT_THUMB);
    bCurrentStates[EGamepadButtonName::LeftThumb]  = IsButtonDown(XINPUT_GAMEPAD_LEFT_THUMB);

    // Shoulders
    bCurrentStates[EGamepadButtonName::RightShoulder] = IsButtonDown(XINPUT_GAMEPAD_RIGHT_SHOULDER);
    bCurrentStates[EGamepadButtonName::LeftShoulder]  = IsButtonDown(XINPUT_GAMEPAD_LEFT_SHOULDER);

    // Start/Back
    bCurrentStates[EGamepadButtonName::Start] = IsButtonDown(XINPUT_GAMEPAD_START);
    bCurrentStates[EGamepadButtonName::Back]  = IsButtonDown(XINPUT_GAMEPAD_BACK);

    // Process digital button states
    for (int32 ButtonIndex = 1; ButtonIndex < NUM_BUTTONS; ButtonIndex++)
    {
        FXInputButtonState& ButtonState = CurrentState.Buttons[ButtonIndex];
        const bool bIsPressed = bCurrentStates[ButtonIndex];

        if (bIsPressed)
        {
            // Button is down
            if (ButtonState.bState) 
            {
                // Already down -> repeat event
                ButtonState.RepeatCount++;
                if (ButtonState.RepeatCount >= RepeatDelay)
                {
                    CurrentMessageHandler->OnGamepadButtonDown(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex, true);

                    // Keep repeat count pegged at RepeatDelay
                    ButtonState.RepeatCount = RepeatDelay;
                }
            }
            else
            {
                // Newly pressed
                CurrentMessageHandler->OnGamepadButtonDown(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex, false );
                ButtonState.RepeatCount = 1;
            }
        }
        else
        {
            // Button is not pressed
            if (ButtonState.bState)
            {
                // Transition from down to up
                CurrentMessageHandler->OnGamepadButtonUp(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex );
                ButtonState.RepeatCount = 0;
            }
        }

        // Update state
        ButtonState.bState = bIsPressed ? 1 : 0;
    }

    // Dispatch analog changes only if needed
    auto DispatchAnalogMessage = [&](EAnalogSourceName::Type AnalogSource, int16 OldValue, int16 NewValue, float NormalizedValue, int16 DeadZone)
    {
        // Always send an update if the value changed; some code chooses to skip if within deadzone
        bool bValueChanged = (OldValue != NewValue);
        if (bValueChanged)
        {
            // Optionally clamp to 0 if inside dead zone
            bool bOutsideDeadZone = (FMath::Abs(NewValue) > DeadZone);
            float FinalValue = bOutsideDeadZone ? NormalizedValue : 0.0f;
            CurrentMessageHandler->OnAnalogGamepadChange(AnalogSource, GamepadIndex, FinalValue);
        }
    };

    // Right Trigger (analog)
    DispatchAnalogMessage(EAnalogSourceName::RightTrigger, CurrentState.RightTrigger, Gamepad.bRightTrigger, NormalizeTrigger(Gamepad.bRightTrigger), XINPUT_GAMEPAD_TRIGGER_THRESHOLD );

    // Left Trigger (analog)
    DispatchAnalogMessage(EAnalogSourceName::LeftTrigger, CurrentState.LeftTrigger, Gamepad.bLeftTrigger, NormalizeTrigger(Gamepad.bLeftTrigger), XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

    // Right Thumb X/Y
    DispatchAnalogMessage(EAnalogSourceName::RightThumbX, CurrentState.RightThumbX, Gamepad.sThumbRX, NormalizeThumbStick(Gamepad.sThumbRX), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    DispatchAnalogMessage(EAnalogSourceName::RightThumbY, CurrentState.RightThumbY, Gamepad.sThumbRY, NormalizeThumbStick(Gamepad.sThumbRY), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

    // Left Thumb X/Y
    DispatchAnalogMessage(EAnalogSourceName::LeftThumbX, CurrentState.LeftThumbX, Gamepad.sThumbLX, NormalizeThumbStick(Gamepad.sThumbLX), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    DispatchAnalogMessage(EAnalogSourceName::LeftThumbY, CurrentState.LeftThumbY, Gamepad.sThumbLY, NormalizeThumbStick(Gamepad.sThumbLY), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

    // Update stored state
    CurrentState.RightThumbX  = Gamepad.sThumbRX;
    CurrentState.RightThumbY  = Gamepad.sThumbRY;
    CurrentState.LeftThumbX   = Gamepad.sThumbLX;
    CurrentState.LeftThumbY   = Gamepad.sThumbLY;
    CurrentState.RightTrigger = Gamepad.bRightTrigger;
    CurrentState.LeftTrigger  = Gamepad.bLeftTrigger;
}
