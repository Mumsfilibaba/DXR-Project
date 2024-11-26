#include "GCInputDevice.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

static TAutoConsoleVariable<int32> CVarGameControllerButtonRepeatDelay(
    "GameController.ButtonRepeatDelay",
    "Number of repeated messages that gets ignored before sending repeat events",
    60,
    EConsoleVariableFlags::Default);

@interface FGCConnectionObserver : NSObject
{
    FGCInputDevice* InputDevice;
}

- (instancetype)initWithInputDevice:(FGCInputDevice*)InInputDevice;
- (void)handleControllerConnect:(NSNotification*)Notification;
- (void)handleControllerDisconnect:(NSNotification*)Notification;

@end

@implementation FGCConnectionObserver

- (instancetype)initWithInputDevice:(FGCInputDevice*)InInputDevice
{
    CHECK(InInputDevice != nullptr);

    self = [super init];
    if (self)
    {
        InputDevice = InInputDevice;
    }

    return self;
}

- (void)handleControllerConnect:(NSNotification*)Notification
{
    GCController* Controller = Notification.object;
    if (Controller)
    {
        InputDevice->HandleControllerConnected(Controller);
    }
}

- (void)handleControllerDisconnect:(NSNotification*)Notification
{
    GCController* Controller = Notification.object;
    if (Controller)
    {
        InputDevice->HandleControllerDisconnected(Controller);
    }
}

@end


TSharedPtr<FGCInputDevice> FGCInputDevice::CreateGCInputDevice()
{
    TSharedPtr<FGCInputDevice> NewInputDevice = MakeSharedPtr<FGCInputDevice>();
    return NewInputDevice;
}

FGCInputDevice::FGCInputDevice()
    : FInputDevice()
    , Observer(nullptr)
    , bIsDeviceConnected(false)
{
    // Ensure that the gamepadstates are starting at zero
    FMemory::Memzero(GamepadStates, sizeof(FGCGamepadState) * NUM_MAX_GAMEPADS);
    
    // Add an observer for new controller connections
    Observer = [[FGCConnectionObserver alloc] initWithInputDevice:this];
    [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(handleControllerConnect:) name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(handleControllerDisconnect:) name:GCControllerDidDisconnectNotification object:nil];
}

FGCInputDevice::~FGCInputDevice()
{
    [[NSNotificationCenter defaultCenter] removeObserver:Observer name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:Observer name:GCControllerDidDisconnectNotification object:nil];
    [Observer release];
}

void FGCInputDevice::UpdateDeviceState()
{
    // Update the state for all the controllers that are connected
    for (int32 Index = 0; Index < ConnectedGamepads.Size(); Index++)
    {
        // TODO: For now only exteneded gamepads are supported
        if (GCExtendedGamepad* ExtendedGamepad = ConnectedGamepads[Index].extendedGamepad)
        {
            ProcessInputState(ExtendedGamepad, Index);
        }
    }
}

void FGCInputDevice::HandleControllerConnected(GCController* InController)
{
    CHECK(InController != nullptr);
    
    // TODO: For now, only extended gamepads are supported, we should look into supporting other types
    GCExtendedGamepad* ExtendedGamepad = InController.extendedGamepad;
    if (ExtendedGamepad && (ConnectedGamepads.Size() < NUM_MAX_GAMEPADS))
    {
        ConnectedGamepads.AddUnique(InController);
        bIsDeviceConnected = true;
    }
}

void FGCInputDevice::HandleControllerDisconnected(GCController* InController)
{
    CHECK(InController != nullptr);
    
    ConnectedGamepads.Remove(InController);
    
    if (ConnectedGamepads.IsEmpty())
    {
        bIsDeviceConnected = false;
    }
}

void FGCInputDevice::ProcessInputState(GCExtendedGamepad* InGamepad, uint32 GamepadIndex)
{
    FGCGamepadState& CurrentState = GamepadStates[GamepadIndex];
    if (TSharedPtr<FGenericApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler())
    {
        // MaxButtonRepeatDelay is based on the number of bits available to the RepeatCount
        constexpr int32 MaxButtonRepeatDelay = (1 << 7) - 1;
        
        // Clamp the repeat delay (TODO: Add support for clamping inside of CVars)
        const int32 RepeatDelay = FMath::Clamp(CVarGameControllerButtonRepeatDelay.GetValue(), 0, MaxButtonRepeatDelay);
        
        // Store the current states
        bool bCurrentStates[EGamepadButtonName::Count];
        FMemory::Memzero(bCurrentStates, sizeof(bCurrentStates));

        bCurrentStates[EGamepadButtonName::DPadUp]        = InGamepad.dpad.up.isPressed;
        bCurrentStates[EGamepadButtonName::DPadDown]      = InGamepad.dpad.down.isPressed;
        bCurrentStates[EGamepadButtonName::DPadLeft]      = InGamepad.dpad.right.isPressed;
        bCurrentStates[EGamepadButtonName::DPadRight]     = InGamepad.dpad.left.isPressed;

        bCurrentStates[EGamepadButtonName::FaceUp]        = InGamepad.buttonY.isPressed;
        bCurrentStates[EGamepadButtonName::FaceDown]      = InGamepad.buttonA.isPressed;
        bCurrentStates[EGamepadButtonName::FaceLeft]      = InGamepad.buttonX.isPressed;
        bCurrentStates[EGamepadButtonName::FaceRight]     = InGamepad.buttonB.isPressed;

        bCurrentStates[EGamepadButtonName::RightTrigger]  = InGamepad.rightThumbstickButton.isPressed;
        bCurrentStates[EGamepadButtonName::LeftTrigger]   = InGamepad.leftThumbstickButton.isPressed;

        bCurrentStates[EGamepadButtonName::RightShoulder] = InGamepad.rightShoulder.isPressed;
        bCurrentStates[EGamepadButtonName::LeftShoulder]  = InGamepad.leftShoulder.isPressed;

        bCurrentStates[EGamepadButtonName::Start]         = InGamepad.buttonMenu.isPressed;
        bCurrentStates[EGamepadButtonName::Back]          = InGamepad.buttonOptions.isPressed;

        for (int32 ButtonIndex = 1; ButtonIndex < EGamepadButtonName::Count; ButtonIndex++)
        {
            FGCButtonState& ButtonState = CurrentState.Buttons[ButtonIndex];
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
        const auto DispatchAnalogMessage = [CurrentMessageHandler](EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float CurrentValue, float NewValue)
        {
            if (CurrentValue != NewValue)
            {
                CurrentMessageHandler->OnAnalogGamepadChange(AnalogSource, GamepadIndex, NewValue);
            }
        };

        // Right Trigger
        DispatchAnalogMessage(EAnalogSourceName::RightTrigger, GamepadIndex, CurrentState.RightTrigger, InGamepad.rightTrigger.value);

        // Left Trigger
        DispatchAnalogMessage(EAnalogSourceName::LeftTrigger, GamepadIndex, CurrentState.LeftTrigger, InGamepad.leftTrigger.value);

        // Right Thumb
        DispatchAnalogMessage(EAnalogSourceName::RightThumbX, GamepadIndex, CurrentState.RightThumbX, InGamepad.rightThumbstick.xAxis.value);
        DispatchAnalogMessage(EAnalogSourceName::RightThumbY, GamepadIndex, CurrentState.RightThumbY, InGamepad.rightThumbstick.yAxis.value);

        // Left Thumb
        DispatchAnalogMessage(EAnalogSourceName::LeftThumbX, GamepadIndex, CurrentState.LeftThumbX, InGamepad.leftThumbstick.xAxis.value);
        DispatchAnalogMessage(EAnalogSourceName::LeftThumbY, GamepadIndex, CurrentState.LeftThumbY, InGamepad.leftThumbstick.yAxis.value);
    }

    CurrentState.RightThumbX  = InGamepad.rightThumbstick.xAxis.value;
    CurrentState.RightThumbY  = InGamepad.rightThumbstick.yAxis.value;
    CurrentState.LeftThumbX   = InGamepad.leftThumbstick.xAxis.value;
    CurrentState.LeftThumbY   = InGamepad.leftThumbstick.yAxis.value;
    CurrentState.RightTrigger = InGamepad.rightTrigger.value;
    CurrentState.LeftTrigger  = InGamepad.leftTrigger.value;
}
