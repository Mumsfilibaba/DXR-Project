#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Mac/GCInputDevice.h"
#include "CoreApplication/PlatformInterface/IPlatformApplicationMessageHandler.h"

static TAutoConsoleVariable<int32> CVarGameControllerButtonRepeatDelay(
    "GameController.ButtonRepeatDelay",
    "Number of repeated messages that gets ignored before sending repeat events",
    60,
    0,
    (1 << 7) - 1,
    EConsoleVariableFlags::Default);

// The XInput thresholds normalized, so a stick behaves the same on both platforms: 7849/32767, 8689/32767 and 30/255
static constexpr float GLeftThumbDeadZone  = 0.24f;
static constexpr float GRightThumbDeadZone = 0.27f;
static constexpr float GTriggerDeadZone    = 0.12f;

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

TSharedPtr<FGCInputDevice> FGCInputDevice::Create()
{
    TSharedPtr<FGCInputDevice> NewInputDevice = MakeSharedPtr<FGCInputDevice>();
    return NewInputDevice;
}

FGCInputDevice::FGCInputDevice()
    : MessageHandler(nullptr)
    , Observer(nullptr)
    , bIsDeviceConnected(false)
{
    Memory::Memzero(ConnectedGamepads, sizeof(ConnectedGamepads));
    Memory::Memzero(GamepadStates, sizeof(FGCGamepadState) * NUM_MAX_GAMEPADS);
    
    Observer = [[FGCConnectionObserver alloc] initWithInputDevice:this];
    [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(handleControllerConnect:) name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(handleControllerDisconnect:) name:GCControllerDidDisconnectNotification object:nil];
}

FGCInputDevice::~FGCInputDevice()
{
    [[NSNotificationCenter defaultCenter] removeObserver:Observer name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:Observer name:GCControllerDidDisconnectNotification object:nil];
    [Observer release];

    SCOPED_LOCK(GamepadsCS);
    for (int32 Index = 0; Index < NUM_MAX_GAMEPADS; Index++)
    {
        [ConnectedGamepads[Index] release];
        ConnectedGamepads[Index] = nullptr;
    }
}

void FGCInputDevice::UpdateDeviceState()
{
    GCController* Snapshot[NUM_MAX_GAMEPADS];
    {
        SCOPED_LOCK(GamepadsCS);
        Memory::Memcpy(Snapshot, ConnectedGamepads, sizeof(Snapshot));
    }

    for (int32 Index = 0; Index < NUM_MAX_GAMEPADS; Index++)
    {
        // TODO: For now only exteneded gamepads are supported
        if (GCExtendedGamepad* ExtendedGamepad = Snapshot[Index].extendedGamepad)
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
    if (!ExtendedGamepad)
    {
        return;
    }

    SCOPED_LOCK(GamepadsCS);

    for (int32 Index = 0; Index < NUM_MAX_GAMEPADS; Index++)
    {
        if (ConnectedGamepads[Index] == InController)
        {
            return;
        }
    }

    for (int32 Index = 0; Index < NUM_MAX_GAMEPADS; Index++)
    {
        if (!ConnectedGamepads[Index])
        {
            ConnectedGamepads[Index] = [InController retain];
            Memory::Memzero(&GamepadStates[Index], sizeof(FGCGamepadState));

            bIsDeviceConnected.Store(true);
            return;
        }
    }
}

void FGCInputDevice::HandleControllerDisconnected(GCController* InController)
{
    CHECK(InController != nullptr);

    SCOPED_LOCK(GamepadsCS);

    bool bAnyConnected = false;
    for (int32 Index = 0; Index < NUM_MAX_GAMEPADS; Index++)
    {
        if (ConnectedGamepads[Index] == InController)
        {
            ReleaseHeldButtons(Index);

            [ConnectedGamepads[Index] release];
            ConnectedGamepads[Index] = nullptr;
            Memory::Memzero(&GamepadStates[Index], sizeof(FGCGamepadState));
        }
        else if (ConnectedGamepads[Index])
        {
            bAnyConnected = true;
        }
    }

    bIsDeviceConnected.Store(bAnyConnected);
}

void FGCInputDevice::ProcessInputState(GCExtendedGamepad* InGamepad, uint32 GamepadIndex)
{
    TSharedPtr<IPlatformApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler();
    if (!CurrentMessageHandler)
    {
        return;
    }

    FGCGamepadState& CurrentState = GamepadStates[GamepadIndex];

    const int32 RepeatDelay = CVarGameControllerButtonRepeatDelay.GetValue();

    bool bCurrentStates[EGamepadButtonName::Count];
    Memory::Memzero(bCurrentStates, sizeof(bCurrentStates));

    bCurrentStates[EGamepadButtonName::DPadUp]    = InGamepad.dpad.up.isPressed;
    bCurrentStates[EGamepadButtonName::DPadDown]  = InGamepad.dpad.down.isPressed;
    bCurrentStates[EGamepadButtonName::DPadLeft]  = InGamepad.dpad.left.isPressed;
    bCurrentStates[EGamepadButtonName::DPadRight] = InGamepad.dpad.right.isPressed;

    bCurrentStates[EGamepadButtonName::FaceUp]    = InGamepad.buttonY.isPressed;
    bCurrentStates[EGamepadButtonName::FaceDown]  = InGamepad.buttonA.isPressed;
    bCurrentStates[EGamepadButtonName::FaceLeft]  = InGamepad.buttonX.isPressed;
    bCurrentStates[EGamepadButtonName::FaceRight] = InGamepad.buttonB.isPressed;

    bCurrentStates[EGamepadButtonName::RightThumb] = InGamepad.rightThumbstickButton.isPressed;
    bCurrentStates[EGamepadButtonName::LeftThumb]  = InGamepad.leftThumbstickButton.isPressed;

    bCurrentStates[EGamepadButtonName::RightShoulder] = InGamepad.rightShoulder.isPressed;
    bCurrentStates[EGamepadButtonName::LeftShoulder]  = InGamepad.leftShoulder.isPressed;

    bCurrentStates[EGamepadButtonName::Start] = InGamepad.buttonMenu.isPressed;
    bCurrentStates[EGamepadButtonName::Back]  = InGamepad.buttonOptions.isPressed;

    for (int32 ButtonIndex = 1; ButtonIndex < EGamepadButtonName::Count; ButtonIndex++)
    {
        FGCButtonState& ButtonState = CurrentState.Buttons[ButtonIndex];
        if (bCurrentStates[ButtonIndex])
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
    const auto DispatchAnalogMessage = [CurrentMessageHandler](EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float CurrentValue, float NewValue, float DeadZone)
    {
        const bool bValueChanged    = (CurrentValue != NewValue);
        const bool bOutsideDeadZone = (Math::Abs(NewValue) > DeadZone);

        if (bValueChanged || bOutsideDeadZone)
        {
            CurrentMessageHandler->OnAnalogGamepadChange(AnalogSource, GamepadIndex, NewValue);
        }
    };

    // Right Trigger
    DispatchAnalogMessage(EAnalogSourceName::RightTrigger, GamepadIndex, CurrentState.RightTrigger, InGamepad.rightTrigger.value, GTriggerDeadZone);

    // Left Trigger
    DispatchAnalogMessage(EAnalogSourceName::LeftTrigger, GamepadIndex, CurrentState.LeftTrigger, InGamepad.leftTrigger.value, GTriggerDeadZone);

    // Right Thumb
    DispatchAnalogMessage(EAnalogSourceName::RightThumbX, GamepadIndex, CurrentState.RightThumbX, InGamepad.rightThumbstick.xAxis.value, GRightThumbDeadZone);
    DispatchAnalogMessage(EAnalogSourceName::RightThumbY, GamepadIndex, CurrentState.RightThumbY, InGamepad.rightThumbstick.yAxis.value, GRightThumbDeadZone);

    // Left Thumb
    DispatchAnalogMessage(EAnalogSourceName::LeftThumbX, GamepadIndex, CurrentState.LeftThumbX, InGamepad.leftThumbstick.xAxis.value, GLeftThumbDeadZone);
    DispatchAnalogMessage(EAnalogSourceName::LeftThumbY, GamepadIndex, CurrentState.LeftThumbY, InGamepad.leftThumbstick.yAxis.value, GLeftThumbDeadZone);

    CurrentState.RightThumbX  = InGamepad.rightThumbstick.xAxis.value;
    CurrentState.RightThumbY  = InGamepad.rightThumbstick.yAxis.value;
    CurrentState.LeftThumbX   = InGamepad.leftThumbstick.xAxis.value;
    CurrentState.LeftThumbY   = InGamepad.leftThumbstick.yAxis.value;
    CurrentState.RightTrigger = InGamepad.rightTrigger.value;
    CurrentState.LeftTrigger  = InGamepad.leftTrigger.value;
}

void FGCInputDevice::ReleaseHeldButtons(uint32 GamepadIndex)
{
    TSharedPtr<IPlatformApplicationMessageHandler> CurrentMessageHandler = GetMessageHandler();
    if (!CurrentMessageHandler)
    {
        return;
    }

    FGCGamepadState& CurrentState = GamepadStates[GamepadIndex];
    for (int32 ButtonIndex = 1; ButtonIndex < EGamepadButtonName::Count; ButtonIndex++)
    {
        FGCButtonState& ButtonState = CurrentState.Buttons[ButtonIndex];
        if (ButtonState.bState)
        {
            CurrentMessageHandler->OnGamepadButtonUp(static_cast<EGamepadButtonName::Type>(ButtonIndex), GamepadIndex);

            ButtonState.bState      = 0;
            ButtonState.RepeatCount = 0;
        }
    }
}
