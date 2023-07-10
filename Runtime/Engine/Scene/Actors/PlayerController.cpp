#include "PlayerController.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"

// TODO: Controll this in some better way than a hardcoded function
static float GetAnalogDeadzone(EAnalogSourceName::Type Source)
{
    switch (Source)
    {
    case EAnalogSourceName::LeftThumbX:
    case EAnalogSourceName::LeftThumbY:
    {
        constexpr float DeadZone = 7849.0f / 32767.0f;
        return DeadZone;
    }
    
    case EAnalogSourceName::RightThumbX:
    case EAnalogSourceName::RightThumbY:
    {
        constexpr float DeadZone = 8689.0f / 32767.0f;
        return DeadZone;
    }

    case EAnalogSourceName::LeftTrigger:
    case EAnalogSourceName::RightTrigger:
    {
        constexpr float DeadZone = 30.0f / 255.0f;
        return DeadZone;
    }

    default:
        return 0.0f;
    }
}

FPlayerInput::FPlayerInput()
    : KeyStates()
    , MouseButtonStates()
    , KeyEvents()
    , MouseEvents()
{
    if (FApplication::IsInitialized())
    {
        CursorInterface = FApplication::Get().GetCursor();
    }
}

void FPlayerInput::Tick(FTimespan Delta)
{
    for (int32 Index = 0; Index < KeyStates.Size();)
    {
        FKeyState& KeyState = KeyStates[Index];
        KeyState.bPreviousState = KeyState.bIsDown;

        if (!KeyState.bIsDown && !KeyState.bPreviousState)
        {
            KeyStates.RemoveAt(Index);
            continue;
        }

        if (KeyState.bIsDown && KeyState.bPreviousState)
        {
            KeyState.TimePressed += Delta.AsMilliseconds();
        }

        Index++;

        LOG_INFO("KeyState(%s)=(bIsDown=%s, bPreviousState=%s, TimePressed=%.4f, RepeatCount=%u)", 
            ToString(KeyState.Key),
            KeyState.bIsDown        ? "true" : "false",
            KeyState.bPreviousState ? "true" : "false",
            KeyState.TimePressed,
            KeyState.RepeatCount);
    }

    for (const FKeyEvent& KeyEvent : KeyEvents)
    {
        int32 Index = KeyStates.FindWithPredicate([&](const FKeyState& KeyState)
        {
            return KeyState.Key == KeyEvent.GetKey();
        });

        if (Index < 0)
        {
            Index = KeyStates.Size();
            KeyStates.Emplace(KeyEvent.GetKey());
        }

        FKeyState& KeyState = KeyStates[Index];
        KeyState.bPreviousState = KeyState.bIsDown;
        KeyState.bIsDown        = KeyEvent.IsDown();

        if (KeyEvent.IsRepeat())
        {
            KeyState.RepeatCount++;
        }
        else
        {
            KeyState.RepeatCount = 0;
        }
    }

    for (int32 Index = 0; Index < ControllerButtonStates.Size();)
    {
        FControllerButtonState& ControllerButtonState = ControllerButtonStates[Index];
        ControllerButtonState.bPreviousState = ControllerButtonState.bIsDown;

        if (!ControllerButtonState.bIsDown && !ControllerButtonState.bPreviousState)
        {
            ControllerButtonStates.RemoveAt(Index);
            continue;
        }

        if (ControllerButtonState.bIsDown && ControllerButtonState.bPreviousState)
        {
            ControllerButtonState.TimePressed += Delta.AsMilliseconds();
        }

        Index++;

        LOG_INFO("ControllerButtonState(%s)=(bIsDown=%s, bPreviousState=%s, TimePressed=%.4f, RepeatCount=%u)",
            ToString(ControllerButtonState.Button),
            ControllerButtonState.bIsDown ? "true" : "false",
            ControllerButtonState.bPreviousState ? "true" : "false",
            ControllerButtonState.TimePressed,
            ControllerButtonState.RepeatCount);
    }

    for (const FControllerEvent& ControllerEvent : ControllerEvents)
    {
        if (ControllerEvent.HasAnalogValue())
        {
            int32 Index = AnalogAxisStates.FindWithPredicate([&](const FAnalogAxisState& AxisState)
            {
                return AxisState.Source == ControllerEvent.GetAnalogSource();
            });

            if (Index < 0)
            {
                Index = AnalogAxisStates.Size();
                AnalogAxisStates.Emplace(ControllerEvent.GetAnalogSource());
            }

            const float DeadZone    = GetAnalogDeadzone(ControllerEvent.GetAnalogSource());
            const float SourceValue = ControllerEvent.GetAnalogValue();

            FAnalogAxisState& AxisState = AnalogAxisStates[Index];
            AxisState.Value               = FMath::Abs(SourceValue) > DeadZone ? SourceValue : 0.0f;
            AxisState.NumTicksSinceUpdate = 0;
        }
        else if (ControllerEvent.GetButton() != EGamepadButtonName::Unknown)
        {
            int32 Index = ControllerButtonStates.FindWithPredicate([&](const FControllerButtonState& ControllerButtonState)
            {
                return ControllerButtonState.Button == ControllerEvent.GetButton();
            });

            if (Index < 0)
            {
                Index = ControllerButtonStates.Size();
                ControllerButtonStates.Emplace(ControllerEvent.GetButton());
            }

            FControllerButtonState& ButtonState = ControllerButtonStates[Index];
            ButtonState.bPreviousState = ButtonState.bIsDown;
            ButtonState.bIsDown        = ControllerEvent.IsButtonDown();

            if (ControllerEvent.IsRepeat())
            {
                ButtonState.RepeatCount++;
            }
            else
            {
                ButtonState.RepeatCount = 0;
            }
        }
    }

    for (int32 Index = 0; Index < AnalogAxisStates.Size(); Index++)
    {
        FAnalogAxisState& AxisState = AnalogAxisStates[Index];
        AxisState.NumTicksSinceUpdate++;
    }

    KeyEvents.Clear();
    MouseEvents.Clear();
    ControllerEvents.Clear();
}

void FPlayerInput::ResetStates()
{
    KeyStates.Clear();

    KeyEvents.Clear();
    MouseEvents.Clear();
    ControllerEvents.Clear();
}

void FPlayerInput::OnControllerEvent(const FControllerEvent& ControllerEvent)
{
    ControllerEvents.Add(ControllerEvent);
}

void FPlayerInput::OnKeyEvent(const FKeyEvent& KeyEvent)
{
    KeyEvents.Add(KeyEvent);
}

void FPlayerInput::OnMouseEvent(const FMouseEvent& MouseEvent)
{
    MouseEvents.Add(MouseEvent);
}

void FPlayerInput::SetCursorPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursorInterface())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

FIntVector2 FPlayerInput::GetCursorPosition() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursorInterface())
    {
        Cursor->GetPosition();
    }

    return FIntVector2(0, 0);
}

FKeyState FPlayerInput::GetKeyState(EKeyName::Type Key) const
{
    const int32 Index = KeyStates.FindWithPredicate([=](const FKeyState& KeyState)
    {
        return KeyState.Key == Key;
    });

    if (Index >= 0)
    {
        return KeyStates[Index];
    }

    return FKeyState(Key);
}

FMouseButtonState FPlayerInput::GetMouseButtonState(EMouseButtonName::Type Button) const
{
    const int32 Index = MouseButtonStates.FindWithPredicate([=](const FMouseButtonState& MouseButtonState)
    {
        return MouseButtonState.Button == Button;
    });

    if (Index >= 0)
    {
        return MouseButtonStates[Index];
    }

    return FMouseButtonState(Button);
}

FControllerButtonState FPlayerInput::GetControllerButtonState(EGamepadButtonName::Type Button) const
{
    const int32 Index = ControllerButtonStates.FindWithPredicate([=](const FControllerButtonState& ControllerButtonState)
    {
        return ControllerButtonState.Button == Button;
    });

    if (Index >= 0)
    {
        return ControllerButtonStates[Index];
    }

    return FControllerButtonState(Button);
}

FAnalogAxisState FPlayerInput::GetAnalogState(EAnalogSourceName::Type Source) const
{
    const int32 Index = AnalogAxisStates.FindWithPredicate([=](const FAnalogAxisState& AxisState)
    {
        return AxisState.Source == Source;
    });

    if (Index >= 0)
    {
        return AnalogAxisStates[Index];
    }

    return FAnalogAxisState(Source);
}


FPlayerController::FPlayerController(FScene* InSceneOwner)
    : FActor(InSceneOwner)
    , InputComponent(nullptr)
    , PlayerInput(nullptr)
{
    FOBJECT_INIT();
    PlayerInput = new FPlayerInput();
}

FPlayerController::~FPlayerController()
{
    SAFE_DELETE(PlayerInput);
}

void FPlayerController::SetupInputComponent()
{
    if (!InputComponent)
    {
        InputComponent = new FInputComponent(this);
        AddComponent(InputComponent);
    }

    CHECK(InputComponent != nullptr);
}

void FPlayerController::Tick(FTimespan DeltaTime)
{
    // Call actor tick
    Super::Tick(DeltaTime);

    // Update the PlayerInput
    PlayerInput->Tick(DeltaTime);
}
