#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Windows/XInputDevice.h"
#include "CoreApplication/PlatformInterface/IPlatformApplicationMessageHandler.h"

static TAutoConsoleVariable<int32> CVarXInputButtonRepeatDelay(
    "XInput.ButtonRepeatDelay",
    "Number of repeated messages that gets ignored before sending repeat events",
    60,
    EConsoleVariableFlags::Default);

TSharedPtr<FXInputDevice> FXInputDevice::Create()
{
    TSharedPtr<FXInputDevice> NewInputDevice = MakeSharedPtr<FXInputDevice>();
    return NewInputDevice;
}

FXInputDevice::FXInputDevice()
    : MessageHandler(nullptr)
    , bIsDeviceConnected(false)
{
    Memory::Memzero(GamepadStates, sizeof(FXInputGamepadState) * XUSER_MAX_COUNT);
}

void FXInputDevice::UpdateDeviceState()
{
    bool bFoundDevice = false;

    for (DWORD GamepadIndex = 0; GamepadIndex < XUSER_MAX_COUNT; ++GamepadIndex)
    {
        FXInputGamepadState& CurrentState = GamepadStates[GamepadIndex];
        if (!CurrentState.bConnected)
        {
            continue;
        }

        XINPUT_STATE State;
        Memory::Memzero(&State, sizeof(XINPUT_STATE));

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

    bIsDeviceConnected = bFoundDevice;
}

void FXInputDevice::UpdateConnectionState()
{
    bool bFoundDevice = false;

    for (DWORD UserIndex = 0; UserIndex < XUSER_MAX_COUNT; ++UserIndex)
    {
        XINPUT_STATE State;
        Memory::Memzero(&State, sizeof(XINPUT_STATE));

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
    TSharedPtr<IPlatformApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler();
    if (!CurrentMessageHandler)
    {
        return;
    }

    FXInputGamepadState& CurrentState = GamepadStates[GamepadIndex];
    const XINPUT_GAMEPAD& Gamepad = State.Gamepad;

    constexpr int32 MaxButtonRepeatDelay = (1 << 7) - 1;
    const int32 RepeatDelay    = Math::Clamp(CVarXInputButtonRepeatDelay.GetValue(), 0, MaxButtonRepeatDelay);
    const int32 GamepadButtons = static_cast<int32>(Gamepad.wButtons);

    auto IsButtonDown = [GamepadButtons](int32 ButtonMask) -> bool
    {
        return (GamepadButtons & ButtonMask) != 0;
    };

    bool bCurrentStates[NUM_BUTTONS];
    Memory::Memzero(bCurrentStates, sizeof(bCurrentStates));

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
            if (ButtonState.bState) 
            {
                ButtonState.RepeatCount++;
                if (ButtonState.RepeatCount >= RepeatDelay)
                {
                    CurrentMessageHandler->OnGamepadButtonDown(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex, true);
                    ButtonState.RepeatCount = RepeatDelay;
                }
            }
            else
            {
                CurrentMessageHandler->OnGamepadButtonDown(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex, false );
                ButtonState.RepeatCount = 1;
            }
        }
        else
        {
            if (ButtonState.bState)
            {
                CurrentMessageHandler->OnGamepadButtonUp(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex );
                ButtonState.RepeatCount = 0;
            }
        }

        ButtonState.bState = bIsPressed ? 1 : 0;
    }

    auto DispatchAnalogMessage = [CurrentMessageHandler](EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, int16 OldValue, int16 NewValue, float NormalizedValue, int16 DeadZone)
    {
        bool bValueChanged    = (OldValue != NewValue);
        bool bOutsideDeadZone = (Math::Abs(NewValue) > DeadZone);

        if (bValueChanged || bOutsideDeadZone)
        {
            CurrentMessageHandler->OnAnalogGamepadChange(AnalogSource, GamepadIndex, NormalizedValue);
        }
    };

    auto NormalizeThumbStick = [](int16 ThumbValue) -> float
    {
        const float ThumbMaxValue = (ThumbValue < 0) ? 32768.0f : 32767.0f;
        return static_cast<float>(ThumbValue) / ThumbMaxValue;
    };

    auto NormalizeTrigger = [](uint8 TriggerValue) -> float
    {
        constexpr float TriggerMaxValue = 255.0f;
        return static_cast<float>(TriggerValue) / TriggerMaxValue;
    };

    // Right Trigger (analog)
    DispatchAnalogMessage(EAnalogSourceName::RightTrigger, GamepadIndex, CurrentState.RightTrigger, Gamepad.bRightTrigger, NormalizeTrigger(Gamepad.bRightTrigger), XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

    // Left Trigger (analog)
    DispatchAnalogMessage(EAnalogSourceName::LeftTrigger, GamepadIndex, CurrentState.LeftTrigger, Gamepad.bLeftTrigger, NormalizeTrigger(Gamepad.bLeftTrigger), XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

    // Right Thumb X/Y
    DispatchAnalogMessage(EAnalogSourceName::RightThumbX, GamepadIndex, CurrentState.RightThumbX, Gamepad.sThumbRX, NormalizeThumbStick(Gamepad.sThumbRX), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    DispatchAnalogMessage(EAnalogSourceName::RightThumbY, GamepadIndex, CurrentState.RightThumbY, Gamepad.sThumbRY, NormalizeThumbStick(Gamepad.sThumbRY), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

    // Left Thumb X/Y
    DispatchAnalogMessage(EAnalogSourceName::LeftThumbX, GamepadIndex, CurrentState.LeftThumbX, Gamepad.sThumbLX, NormalizeThumbStick(Gamepad.sThumbLX), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    DispatchAnalogMessage(EAnalogSourceName::LeftThumbY, GamepadIndex, CurrentState.LeftThumbY, Gamepad.sThumbLY, NormalizeThumbStick(Gamepad.sThumbLY), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

    CurrentState.RightThumbX  = Gamepad.sThumbRX;
    CurrentState.RightThumbY  = Gamepad.sThumbRY;
    CurrentState.LeftThumbX   = Gamepad.sThumbLX;
    CurrentState.LeftThumbY   = Gamepad.sThumbLY;
    CurrentState.RightTrigger = Gamepad.bRightTrigger;
    CurrentState.LeftTrigger  = Gamepad.bLeftTrigger;
}
