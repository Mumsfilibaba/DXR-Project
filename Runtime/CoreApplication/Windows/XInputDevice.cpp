#include "XInputDevice.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformTime.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

namespace XInputPrivate
{
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

    FORCEINLINE static void DispatchAnalogMessage(
        const TSharedPtr<FGenericApplicationMessageHandler>& MessageHandler,
        EControllerAnalog                                    AnalogSource,
        uint32                                               ControllerIndex,
        int16                                                CurrentValue,
        int16                                                NewValue,
        float                                                NormalizedValue,
        int16                                                DeadZone)
    {
        if (CurrentValue != NewValue || static_cast<int16>(FMath::Abs(NewValue)) > DeadZone)
        {
            MessageHandler->OnControllerAnalog(AnalogSource, ControllerIndex, NormalizedValue);
        }
    }
}

FXInputDevice::FXInputDevice()
    : FInputDevice()
    , Frequency{0}
    , LastConnectionPollTimeStamp{0}
    , LastPollTimeStamp{0}
{
    FMemory::Memzero(ControllerState, sizeof(FXInputControllerState) * MAX_CONTROLLERS);
    Frequency = FPlatformTime::QueryPerformanceFrequency();
}

void FXInputDevice::UpdateDeviceState()
{
    constexpr int64 MicrosecondsDiff = 1'000'000;
    const int64 CurrentTimeStamp = FPlatformTime::QueryPerformanceCounter();

    int64 ElapsedTime = (CurrentTimeStamp - LastPollTimeStamp) * MicrosecondsDiff;
    ElapsedTime = ElapsedTime / Frequency;

    // Check the known devices every millisecond
    if (ElapsedTime > 1000)
    {
        UpdateConnectedDevices();
        LastPollTimeStamp = CurrentTimeStamp;
    }
}

bool FXInputDevice::IsDeviceConnected() const
{
    for (int32 UserIndex = 0; UserIndex < MAX_CONTROLLERS; UserIndex++)
    {
        if (ControllerState[UserIndex].bConnected)
        {
            return true;
        }
    }

    return false;
}

void FXInputDevice::CheckForNewConnections()
{
    DWORD Result = ERROR_DEVICE_NOT_CONNECTED;
    for (DWORD UserIndex = 0; UserIndex < MAX_CONTROLLERS; UserIndex++)
    {
        XINPUT_STATE State;
        FMemory::Memzero(&State, sizeof(XINPUT_STATE));

        Result = XInputGetState(UserIndex, &State);
        if (Result == ERROR_SUCCESS)
        {
            ControllerState[UserIndex].bConnected = true;
        }
        else
        {
            ControllerState[UserIndex].bConnected = false;
        }
    }
}

void FXInputDevice::UpdateConnectedDevices()
{
    DWORD Result = ERROR_DEVICE_NOT_CONNECTED;
    for (DWORD ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ControllerIndex++)
    {
        FXInputControllerState& CurrentState = ControllerState[ControllerIndex];
        if (CurrentState.bConnected)
        {
            XINPUT_STATE State;
            FMemory::Memzero(&State, sizeof(XINPUT_STATE));

            Result = XInputGetState(ControllerIndex, &State);
            if (Result == ERROR_SUCCESS)
            {
                // Check if the state of the controller has changed, if it has then process it
                if (CurrentState.LastPacketNumber != State.dwPacketNumber)
                {
                    ProcessInputState(State, ControllerIndex);
                    CurrentState.LastPacketNumber = State.dwPacketNumber;
                }
            }
            else
            {
                ControllerState[ControllerIndex].bConnected = false;
            }
        }
    }
}

void FXInputDevice::ProcessInputState(const XINPUT_STATE& State, uint32 ControllerIndex)
{
    const XINPUT_GAMEPAD& Gamepad = State.Gamepad;

    FXInputControllerState& CurrentState = ControllerState[ControllerIndex];
    if (TSharedPtr<FGenericApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler())
    {
        const uint16 CurrentButtonState = CurrentState.Buttons.ButtonState;
        const uint16 NewButtonState = Gamepad.wButtons;
        const uint16 ButtonStateMask = ~(CurrentButtonState & NewButtonState);

        FXInputButtonState DownButtonState = (NewButtonState & ButtonStateMask);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_DPAD_UP))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::DPadUp, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_DPAD_DOWN))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::DPadDown, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_DPAD_LEFT))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::DPadLeft, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_DPAD_RIGHT))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::DPadRight, ControllerIndex);

        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_A))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::FaceDown, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_B))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::FaceRight, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_Y))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::FaceUp, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_X))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::FaceLeft, ControllerIndex);

        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_RIGHT_THUMB))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::RightTrigger, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_LEFT_THUMB))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::LeftTrigger, ControllerIndex);

        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_RIGHT_SHOULDER))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::RightShoulder, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_LEFT_SHOULDER))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::LeftShoulder, ControllerIndex);

        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_START))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::Start, ControllerIndex);
        if (DownButtonState.IsButtonDown(XINPUT_GAMEPAD_BACK))
            CurrentMessageHandler->OnControllerButtonDown(EControllerButton::Back, ControllerIndex);

        // Button up state must be "reversed" since bit that is set means down
        FXInputButtonState UpButtonState = ~(CurrentButtonState & (~NewButtonState));
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_DPAD_UP))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::DPadUp, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_DPAD_DOWN))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::DPadDown, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_DPAD_LEFT))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::DPadLeft, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_DPAD_RIGHT))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::DPadRight, ControllerIndex);

        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_A))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::FaceDown, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_B))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::FaceRight, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_Y))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::FaceUp, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_X))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::FaceLeft, ControllerIndex);

        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_RIGHT_THUMB))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::RightTrigger, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_LEFT_THUMB))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::LeftTrigger, ControllerIndex);

        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_RIGHT_SHOULDER))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::RightShoulder, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_LEFT_SHOULDER))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::LeftShoulder, ControllerIndex);

        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_START))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::Start, ControllerIndex);
        if (UpButtonState.IsButtonUp(XINPUT_GAMEPAD_BACK))
            CurrentMessageHandler->OnControllerButtonUp(EControllerButton::Back, ControllerIndex);

        // Right Trigger
        XInputPrivate::DispatchAnalogMessage(
            CurrentMessageHandler,
            EControllerAnalog::RightTrigger,
            ControllerIndex,
            CurrentState.RightTrigger,
            Gamepad.bRightTrigger,
            XInputPrivate::NormalizeTrigger(Gamepad.bRightTrigger),
            GAMEPAD_TRIGGER_THRESHOLD);

        // Left Trigger
        XInputPrivate::DispatchAnalogMessage(
            CurrentMessageHandler,
            EControllerAnalog::LeftTrigger,
            ControllerIndex,
            CurrentState.LeftTrigger,
            Gamepad.bLeftTrigger,
            XInputPrivate::NormalizeTrigger(Gamepad.bLeftTrigger),
            GAMEPAD_TRIGGER_THRESHOLD);

        // Right Thumb
        XInputPrivate::DispatchAnalogMessage(
            CurrentMessageHandler,
            EControllerAnalog::RightThumbX,
            ControllerIndex,
            CurrentState.RightThumbX,
            Gamepad.sThumbRX,
            XInputPrivate::NormalizeThumbStick(Gamepad.sThumbRX),
            GAMEPAD_RIGHT_THUMB_DEADZONE);
        XInputPrivate::DispatchAnalogMessage(
            CurrentMessageHandler,
            EControllerAnalog::RightThumbY,
            ControllerIndex,
            CurrentState.RightThumbY,
            Gamepad.sThumbRY,
            XInputPrivate::NormalizeThumbStick(Gamepad.sThumbRY),
            GAMEPAD_RIGHT_THUMB_DEADZONE);

        // Left Thumb
        XInputPrivate::DispatchAnalogMessage(
            CurrentMessageHandler,
            EControllerAnalog::LeftThumbX,
            ControllerIndex,
            CurrentState.LeftThumbX,
            Gamepad.sThumbLX,
            XInputPrivate::NormalizeThumbStick(Gamepad.sThumbLX),
            GAMEPAD_LEFT_THUMB_DEADZONE);
        XInputPrivate::DispatchAnalogMessage(
            CurrentMessageHandler,
            EControllerAnalog::LeftThumbY,
            ControllerIndex,
            CurrentState.LeftThumbY,
            Gamepad.sThumbLY,
            XInputPrivate::NormalizeThumbStick(Gamepad.sThumbLY),
            GAMEPAD_LEFT_THUMB_DEADZONE);
    }

    CurrentState.Buttons      = Gamepad.wButtons;
    CurrentState.RightThumbX  = Gamepad.sThumbRX;
    CurrentState.RightThumbY  = Gamepad.sThumbRY;
    CurrentState.LeftThumbX   = Gamepad.sThumbLX;
    CurrentState.LeftThumbY   = Gamepad.sThumbLY;
    CurrentState.RightTrigger = Gamepad.bRightTrigger;
    CurrentState.LeftTrigger  = Gamepad.bLeftTrigger;
}
