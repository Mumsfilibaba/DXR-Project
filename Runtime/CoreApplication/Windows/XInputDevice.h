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

struct FXInputButtonState
{
    // Button is either pressed or released (1 or 0)
    uint8 bState      : 1;
    uint8 RepeatCount : 7;
};

struct FXInputGamepadState
{
    int16 LeftThumbX;
    int16 LeftThumbY;

    int16 RightThumbX;
    int16 RightThumbY;

    uint8 LeftTrigger;
    uint8 RightTrigger;
    bool  bConnected;

    FXInputButtonState Buttons[NUM_BUTTONS];
};

class FXInputDevice : public FInputDevice
{
public:
    FXInputDevice();
    virtual ~FXInputDevice() = default;

    virtual void UpdateDeviceState() override final;

    virtual bool IsDeviceConnected() const override final
    {
        return bIsDeviceConnected;
    }

    void UpdateConnectionState();

private:
    void ProcessInputState(const XINPUT_STATE& State, uint32 GamepadIndex);

    FXInputGamepadState GamepadStates[XUSER_MAX_COUNT];
    bool bIsDeviceConnected;
};