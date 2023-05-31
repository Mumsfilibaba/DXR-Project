#pragma once
#include "CoreApplication/Generic/InputDevice.h"

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
    #include <XInput.h>
    #pragma comment(lib,"xinput.lib")
#else
    #include <XInput.h>
    #pragma comment(lib,"xinput9_1_0.lib")
#endif

#define GAMEPAD_LEFT_THUMB_DEADZONE  (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
#define GAMEPAD_RIGHT_THUMB_DEADZONE (XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
#define GAMEPAD_TRIGGER_THRESHOLD    (XINPUT_GAMEPAD_TRIGGER_THRESHOLD)

#define MAX_CONTROLLERS (XUSER_MAX_COUNT)

struct FXInputButtonState
{
    FXInputButtonState() = default;

    FXInputButtonState(uint16 InButtonState)
        : ButtonState{InButtonState}
    {
    }

    bool IsButtonDown(uint16 Mask) const
    {
        return (ButtonState & Mask) != 0;
    }

    bool IsButtonUp(uint16 Mask) const
    {
        return !IsButtonDown(Mask);
    }

    FXInputButtonState& operator=(uint16 InButtonState)
    {
        ButtonState = InButtonState;
        return *this;
    }

    uint16 ButtonState{0};
};

struct FXInputControllerState
{
    uint32 LastPacketNumber;
    
    FXInputButtonState Buttons;
    
    bool   bConnected;
    uint8  LeftTrigger;
    uint8  RightTrigger;

    int16  LeftThumbX;
    int16  LeftThumbY;
    
    int16  RightThumbX;
    int16  RightThumbY;
};

class FXInputDevice : public FInputDevice
{
public:
    FXInputDevice();
    ~FXInputDevice();

    virtual void PollDeviceState() override final;

    void PollForNewConnections();
    void PollConnectedDevices();

private:
    void ProcessInputState(const XINPUT_STATE& State, uint32 ControllerIndex);
    void ProcessButtonDown(FGenericApplicationMessageHandler* MessageHandler, uint32 ControllerIndex, uint16 CurrentButtonState, uint16 NewButtonState);
    void ProcessButtonUp  (FGenericApplicationMessageHandler* MessageHandler, uint32 ControllerIndex, uint16 CurrentButtonState, uint16 NewButtonState);
    void ProcessThumbstick(FGenericApplicationMessageHandler* MessageHandler, uint32 ControllerIndex, EControllerAnalog AnalogSource, uint16 ThumbStickValue);

    FXInputControllerState ControllerState[MAX_CONTROLLERS];

    int64 LastPollTimeStamp;
    int64 LastConnectionPollTimeStamp;
    int64 Frequency;
};