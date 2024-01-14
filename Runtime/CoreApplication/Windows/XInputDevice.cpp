#include "XInputDevice.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

static TAutoConsoleVariable<int32> CVarXInputButtonRepeatDelay(
    "XInput.ButtonRepeatDelay",
    "Number of repeated messages that gets ignored before sending repeat events",
    60,
    EConsoleVariableFlags::Default);

FORCEINLINE static float NormalizeThumbStick(int16 ThumbValue)
{
    const float ThumbMaxValue = (ThumbValue <= 0) ? 32768.0f : 32767.0f;
    return static_cast<float>(ThumbValue) / ThumbMaxValue;
}

FORCEINLINE static float NormalizeTrigger(uint8 TriggerValue)
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

    // Update the state for all the controllers that are connected
    DWORD Result = ERROR_DEVICE_NOT_CONNECTED;
    for (DWORD GamepadIndex = 0; GamepadIndex < XUSER_MAX_COUNT; GamepadIndex++)
    {
        FXInputGamepadState& CurrentState = GamepadStates[GamepadIndex];
        if (!CurrentState.bConnected)
        {
            continue;
        }

        XINPUT_STATE State;
        FMemory::Memzero(&State, sizeof(XINPUT_STATE));

        Result = XInputGetState(GamepadIndex, &State);
        if (Result == ERROR_SUCCESS)
        {
            ProcessInputState(State, GamepadIndex);
            bFoundDevice = true;
        }
        else
        {
            GamepadStates[GamepadIndex].bConnected = false;
        }
    }

    // Update the current device state
    bIsDeviceConnected = bFoundDevice;
}

void FXInputDevice::UpdateConnectionState()
{
    bool bFoundDevice = false;

    // Check if there is any controller connected
    DWORD Result = ERROR_DEVICE_NOT_CONNECTED;
    for (DWORD UserIndex = 0; UserIndex < XUSER_MAX_COUNT; UserIndex++)
    {
        XINPUT_STATE State;
        FMemory::Memzero(&State, sizeof(XINPUT_STATE));

        Result = XInputGetState(UserIndex, &State);
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

    // Update the current device state
    bIsDeviceConnected = bFoundDevice;
}

void FXInputDevice::ProcessInputState(const XINPUT_STATE& State, uint32 GamepadIndex)
{
    const XINPUT_GAMEPAD& Gamepad = State.Gamepad;

    FXInputGamepadState& CurrentState = GamepadStates[GamepadIndex];
    if (TSharedPtr<FGenericApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler())
    {
        // MaxButtonRepeatDelay is based on the number of bits available to the RepeatCount
        constexpr int32 MaxButtonRepeatDelay = (1 << 7) - 1;
        
        // Clamp the repeat delay (TODO: Add support for clamping inside of CVars)
        const int32 RepeatDelay    = FMath::Clamp(0, MaxButtonRepeatDelay, CVarXInputButtonRepeatDelay.GetValue());
        const int32 GamePadButtons = Gamepad.wButtons;

        const auto IsButtonDown = [GamePadButtons](int32 ButtonMask) -> bool
        {
            return (GamePadButtons & ButtonMask) != 0;
        };
        
        // Store the current states
        bool bCurrentStates[NUM_BUTTONS];
        FMemory::Memzero(bCurrentStates, sizeof(bCurrentStates));

        bCurrentStates[EGamepadButtonName::DPadUp]        = IsButtonDown(XINPUT_GAMEPAD_DPAD_UP);
        bCurrentStates[EGamepadButtonName::DPadDown]      = IsButtonDown(XINPUT_GAMEPAD_DPAD_DOWN);
        bCurrentStates[EGamepadButtonName::DPadLeft]      = IsButtonDown(XINPUT_GAMEPAD_DPAD_LEFT);
        bCurrentStates[EGamepadButtonName::DPadRight]     = IsButtonDown(XINPUT_GAMEPAD_DPAD_RIGHT);

        bCurrentStates[EGamepadButtonName::FaceUp]        = IsButtonDown(XINPUT_GAMEPAD_Y);
        bCurrentStates[EGamepadButtonName::FaceDown]      = IsButtonDown(XINPUT_GAMEPAD_A);
        bCurrentStates[EGamepadButtonName::FaceLeft]      = IsButtonDown(XINPUT_GAMEPAD_X);
        bCurrentStates[EGamepadButtonName::FaceRight]     = IsButtonDown(XINPUT_GAMEPAD_B);

        bCurrentStates[EGamepadButtonName::RightTrigger]  = IsButtonDown(XINPUT_GAMEPAD_RIGHT_THUMB);
        bCurrentStates[EGamepadButtonName::LeftTrigger]   = IsButtonDown(XINPUT_GAMEPAD_LEFT_THUMB);

        bCurrentStates[EGamepadButtonName::RightShoulder] = IsButtonDown(XINPUT_GAMEPAD_RIGHT_SHOULDER);
        bCurrentStates[EGamepadButtonName::LeftShoulder]  = IsButtonDown(XINPUT_GAMEPAD_LEFT_SHOULDER);

        bCurrentStates[EGamepadButtonName::Start]         = IsButtonDown(XINPUT_GAMEPAD_START);
        bCurrentStates[EGamepadButtonName::Back]          = IsButtonDown(XINPUT_GAMEPAD_BACK);

        for (int32 ButtonIndex = 1; ButtonIndex < NUM_BUTTONS; ButtonIndex++)
        {
            FXInputButtonState& ButtonState = CurrentState.Buttons[ButtonIndex];
            if (bCurrentStates[ButtonIndex])
            {
                // If the button already is down, this is a repeat event
                if (ButtonState.bState)
                {
                    ButtonState.RepeatCount++;

                    // Only send repeat events after some time
                    if (ButtonState.RepeatCount >= RepeatDelay)
                    {
                        CurrentMessageHandler->OnGamepadButtonDown(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex, true);
                        ButtonState.RepeatCount = RepeatDelay;
                    }
                }
                else
                {
                    CurrentMessageHandler->OnGamepadButtonDown(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex, false);
                    ButtonState.RepeatCount = 1;
                }
            }
            else if (ButtonState.bState)
            {
                CurrentMessageHandler->OnGamepadButtonUp(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex);
                ButtonState.RepeatCount = 0;
            }

            ButtonState.bState = bCurrentStates[ButtonIndex] ? 1 : 0;
        }

        // Handle Analog States
        const auto DispatchAnalogMessage = [CurrentMessageHandler](EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, int16 CurrentValue, int16 NewValue, float NormalizedValue, int16 DeadZone)
        {
            if (CurrentValue != NewValue || FMath::Abs(NewValue) > DeadZone)
            {
                CurrentMessageHandler->OnAnalogGamepadChange(AnalogSource, GamepadIndex, NormalizedValue);
            }
        };

        // Right Trigger
        DispatchAnalogMessage(EAnalogSourceName::RightTrigger, GamepadIndex, CurrentState.RightTrigger, Gamepad.bRightTrigger, NormalizeTrigger(Gamepad.bRightTrigger), XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

        // Left Trigger
        DispatchAnalogMessage(EAnalogSourceName::LeftTrigger, GamepadIndex, CurrentState.LeftTrigger, Gamepad.bLeftTrigger, NormalizeTrigger(Gamepad.bLeftTrigger), XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

        // Right Thumb
        DispatchAnalogMessage(EAnalogSourceName::RightThumbX, GamepadIndex, CurrentState.RightThumbX, Gamepad.sThumbRX, NormalizeThumbStick(Gamepad.sThumbRX), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        DispatchAnalogMessage(EAnalogSourceName::RightThumbY, GamepadIndex, CurrentState.RightThumbY, Gamepad.sThumbRY, NormalizeThumbStick(Gamepad.sThumbRY), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

        // Left Thumb
        DispatchAnalogMessage(EAnalogSourceName::LeftThumbX, GamepadIndex, CurrentState.LeftThumbX, Gamepad.sThumbLX, NormalizeThumbStick(Gamepad.sThumbLX), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        DispatchAnalogMessage(EAnalogSourceName::LeftThumbY, GamepadIndex, CurrentState.LeftThumbY, Gamepad.sThumbLY, NormalizeThumbStick(Gamepad.sThumbLY), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }

    CurrentState.RightThumbX  = Gamepad.sThumbRX;
    CurrentState.RightThumbY  = Gamepad.sThumbRY;
    CurrentState.LeftThumbX   = Gamepad.sThumbLX;
    CurrentState.LeftThumbY   = Gamepad.sThumbLY;
    CurrentState.RightTrigger = Gamepad.bRightTrigger;
    CurrentState.LeftTrigger  = Gamepad.bLeftTrigger;
}
