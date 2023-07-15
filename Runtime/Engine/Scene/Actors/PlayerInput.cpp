#include "PlayerInput.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Engine/Scene/Components/InputComponent.h"

// TODO: Control this in some better way than a hardcoded function
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
{
    if (FApplication::IsInitialized())
    {
        CursorInterface = FApplication::Get().GetCursor();
    }
}

void FPlayerInput::Tick(FTimespan Delta)
{
    TArray<FInputActionDelegate> InputActionsToCall;
 
    // Update all keys
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
            KeyState.TimePressed += static_cast<float>(Delta.AsMilliseconds());
        }

        // Check all the actions mappings if they should be invoked
        for (const FActionKeyMapping& KeyMapping : ActionKeyMappings)
        {
            if (KeyMapping.Key == KeyState.Key)
            {
                for (FInputComponent* InputComponent : ActiveInputComponents)
                {
                    for (const FActionInputBinding& ActionBinding : InputComponent->ActionBindings)
                    {
                        if (ActionBinding.Name == KeyMapping.Name)
                        {
                            if (ActionBinding.ActionState == EActionState::Pressed)
                            {
                                // NOTE: Key is down
                                if (KeyState.bIsDown)
                                {
                                    InputActionsToCall.Add(ActionBinding.ActionDelegate);
                                }
                            }
                            else if (ActionBinding.ActionState == EActionState::Released)
                            {
                                // NOTE: Key was just released
                                if (!KeyState.bIsDown && KeyState.bPreviousState)
                                {
                                    InputActionsToCall.Add(ActionBinding.ActionDelegate);
                                }
                            }
                        }
                    }
                }
            }
        }

        Index++;
    }

    // Update all axis
    for (int32 Index = 0; Index < AnalogAxisStates.Size(); Index++)
    {
        FAnalogAxisState& AxisState = AnalogAxisStates[Index];
        AxisState.NumTicksSinceUpdate++;
    }

    // NOTE: Execute all the action delegates that are "active"
    for (const FInputActionDelegate& Delegate : InputActionsToCall)
    {
        Delegate.ExecuteIfBound();
    }

    ActiveInputComponents.Clear();
}

void FPlayerInput::EnableInput(FInputComponent* InputComponent)
{
    if (InputComponent)
    {
        ActiveInputComponents.Add(InputComponent);
    }
}

void FPlayerInput::ClearInputStates()
{
    KeyStates.Clear();
}

void FPlayerInput::AddActionKeyMapping(const FActionKeyMapping& ActionKeyMapping)
{
    ActionKeyMappings.Add(ActionKeyMapping);
}

void FPlayerInput::SetCursorPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursorInterface())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

void FPlayerInput::OnAnalogGamepadEvent(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    int32 Index = AnalogAxisStates.FindWithPredicate([&](const FAnalogAxisState& AxisState)
    {
        return AxisState.Source == AnalogGamepadEvent.GetAnalogSource();
    });

    if (Index < 0)
    {
        Index = AnalogAxisStates.Size();
        AnalogAxisStates.Emplace(AnalogGamepadEvent.GetAnalogSource());
    }

    const float DeadZone    = GetAnalogDeadzone(AnalogGamepadEvent.GetAnalogSource());
    const float SourceValue = AnalogGamepadEvent.GetAnalogValue();

    FAnalogAxisState& AxisState = AnalogAxisStates[Index];
    AxisState.Value = FMath::Abs(SourceValue) > DeadZone ? SourceValue : 0.0f;
    AxisState.NumTicksSinceUpdate = 0;
}

void FPlayerInput::OnKeyEvent(const FKeyEvent& KeyEvent)
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
    KeyState.bIsDown = KeyEvent.IsDown();

    LOG_INFO("KeyState=%s Index=%d IsDown=%s", KeyEvent.GetKey().ToString(), Index, KeyEvent.IsDown() ? "true" : "false");

    if (KeyEvent.IsRepeat())
    {
        KeyState.RepeatCount++;
    }
    else
    {
        KeyState.RepeatCount = 0;
    }
}

void FPlayerInput::OnCursorEvent(const FCursorEvent& CursorEvent)
{
    // TODO: Handle mouse movement
}

FIntVector2 FPlayerInput::GetCursorPosition() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursorInterface())
    {
        Cursor->GetPosition();
    }

    return FIntVector2(0, 0);
}

FKeyState FPlayerInput::GetKeyState(FKey Key) const
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