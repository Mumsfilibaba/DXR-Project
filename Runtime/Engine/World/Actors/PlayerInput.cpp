#include "PlayerInput.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Engine/World/Components/InputComponent.h"

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

        // Update the previous state last, otherwise we don't catch cases where we just released the buttona
        KeyState.bPreviousState = KeyState.bIsDown;
        Index++;
    }

    // Update all axis
    for (int32 Index = 0; Index < AxisStates.Size(); Index++)
    {
        FAxisState& AxisState = AxisStates[Index];
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

int32 FPlayerInput::AddActionKeyMapping(const FActionKeyMapping& ActionKeyMapping)
{
    const int32 MappingIndex = ActionKeyMappings.Size();
    ActionKeyMappings.Add(ActionKeyMapping);
    return MappingIndex;
}

int32 FPlayerInput::AddAxisMapping(const FAxisMapping& AxisMapping)
{
    const int32 MappingIndex = AxisMappings.Size();
    AxisMappings.Add(AxisMapping);
    return MappingIndex;
}

int32 FPlayerInput::AddAxisKeyMapping(const FAxisKeyMapping& AxisKeyMapping)
{
    const int32 MappingIndex = AxisKeyMappings.Size();
    AxisKeyMappings.Add(AxisKeyMapping);
    return MappingIndex;
}

void FPlayerInput::SetCursorPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursorInterface())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

void FPlayerInput::OnAxisEvent(EAnalogSourceName::Type AxisSource, float AxisValue)
{
    int32 Index = AxisStates.FindWithPredicate([=](const FAxisState& AxisState)
    {
        return AxisState.Source == AxisSource;
    });

    if (Index < 0)
    {
        Index = AxisStates.Size();
        AxisStates.Emplace(AxisSource);
    }

    const float DeadZone = GetAnalogDeadzone(AxisSource);

    FAxisState& AxisState = AxisStates[Index];
    AxisState.Value = FMath::Abs(AxisValue) > DeadZone ? AxisValue : 0.0f;
    AxisState.NumTicksSinceUpdate = 0;
}

void FPlayerInput::OnKeyEvent(FKey Key, bool bIsDown, bool bIsRepeat)
{
    int32 Index = KeyStates.FindWithPredicate([=](const FKeyState& KeyState)
    {
        return KeyState.Key == Key;
    });

    if (Index < 0)
    {
        Index = KeyStates.Size();
        KeyStates.Emplace(Key);
    }

    FKeyState& KeyState = KeyStates[Index];
    KeyState.bPreviousState = KeyState.bIsDown;
    KeyState.bIsDown        = bIsDown;

    // LOG_INFO("KeyState=%s Index=%d IsDown=%s", Key.ToString(), Index, bIsDown ? "true" : "false");

    if (bIsRepeat)
    {
        KeyState.RepeatCount++;
    }
    else
    {
        KeyState.RepeatCount = 0;
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

FAxisState FPlayerInput::GetAnalogState(EAnalogSourceName::Type Source) const
{
    const int32 Index = AxisStates.FindWithPredicate([=](const FAxisState& AxisState)
    {
        return AxisState.Source == Source;
    });

    if (Index >= 0)
    {
        return AxisStates[Index];
    }

    return FAxisState(Source);
}
