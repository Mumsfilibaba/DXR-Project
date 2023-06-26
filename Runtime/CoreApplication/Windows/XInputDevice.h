#pragma once
#include "Core/Input/InputCodes.h"
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

struct FXInputControllerState
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

    void CheckForNewDevices();

private:
    void ProcessInputState(const XINPUT_STATE& State, uint32 ControllerIndex);
    
    void ProcessButtonDown(FGenericApplicationMessageHandler* MessageHandler, uint32 ControllerIndex, uint16 CurrentButtonState, uint16 NewButtonState);
    
    void ProcessButtonUp(FGenericApplicationMessageHandler* MessageHandler, uint32 ControllerIndex, uint16 CurrentButtonState, uint16 NewButtonState);
    
    void ProcessThumbstick(FGenericApplicationMessageHandler* MessageHandler, uint32 ControllerIndex, EAnalogSourceName AnalogSource, uint16 ThumbStickValue);

    FXInputControllerState ControllerState[XUSER_MAX_COUNT];
    bool bIsDeviceConnected;
};