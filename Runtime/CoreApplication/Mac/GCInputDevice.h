#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/InputDevice.h"

#include <GameController/GameController.h>

#define NUM_MAX_GAMEPADS (4)

struct FGCButtonState
{
    // Button is either pressed or released (1 or 0)
    uint8 bState      : 1;
    uint8 RepeatCount : 7;
};

struct FGCGamepadState
{
    float LeftThumbX;
    float LeftThumbY;

    float RightThumbX;
    float RightThumbY;

    float LeftTrigger;
    float RightTrigger;

    FGCButtonState Buttons[EGamepadButtonName::Count];
};

@class FGCConnectionObserver;

class FGCInputDevice : public FInputDevice
{
public:
    static TSharedPtr<FGCInputDevice> CreateGCInputDevice();

    FGCInputDevice();
    virtual ~FGCInputDevice();
    
    virtual void UpdateDeviceState() override final;

    virtual bool IsDeviceConnected() const override final
    {
        return bIsDeviceConnected;
    }

    void HandleControllerConnected(GCController* InController);
    void HandleControllerDisconnected(GCController* InController);
    
private:
    void ProcessInputState(GCExtendedGamepad* InGamepad, uint32 GamepadIndex);

    FGCConnectionObserver* Observer;
    TArray<GCController*>  ConnectedGamepads;
    FGCGamepadState        GamepadStates[NUM_MAX_GAMEPADS];
    bool                   bIsDeviceConnected;
};
