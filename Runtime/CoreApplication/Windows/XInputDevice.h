#pragma once
#include "CoreApplication/PlatformInterface/InputCodes.h"
#include "CoreApplication/PlatformInterface/IPlatformInputDevice.h"

#if (_WIN32_WINNT >= 0x0602)
    #include <XInput.h>
    #pragma comment(lib,"xinput.lib")
#else
    #include <XInput.h>
    #pragma comment(lib,"xinput9_1_0.lib")
#endif

#define NUM_BUTTONS (15)

struct FXInputButtonState
{
    /** @brief Indicates if the button is pressed (1) or released (0). */
    uint8 bState : 1;

    /** @brief The number of consecutive frames that the button has been pressed. */
    uint8 RepeatCount : 7;
};

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

class FXInputDevice : public IPlatformInputDevice
{
public:
    static TSharedPtr<FXInputDevice> Create();

public:
    FXInputDevice();
    virtual ~FXInputDevice() = default;

    void UpdateConnectionState();

    // IPlatformInputDevice Interface
    virtual void UpdateDeviceState() override final;

    virtual bool IsDeviceConnected() const override final
    {
        return bIsDeviceConnected;
    }

    virtual void SetMessageHandler(const TSharedPtr<IPlatformApplicationMessageHandler>& InMessageHandler) override final
    {
        MessageHandler = InMessageHandler;
    }

    virtual TSharedPtr<IPlatformApplicationMessageHandler> GetMessageHandler() const override final
    {
        return MessageHandler;
    }

private:
    void ProcessInputState(const XINPUT_STATE& State, uint32 GamepadIndex);

    FXInputGamepadState                            GamepadStates[XUSER_MAX_COUNT];
    bool                                           bIsDeviceConnected;
    TSharedPtr<IPlatformApplicationMessageHandler> MessageHandler;
};
