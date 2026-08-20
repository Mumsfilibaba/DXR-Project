#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Containers/Pair.h"
#include "CoreApplication/PlatformInterface/AnalogDeadzones.h"
#include "Application/Application.h"
#include "Engine/World/Actors/PlayerInput.h"
#include "Engine/World/Components/InputComponent.h"

FPlayerInput::FPlayerInput()
    : MouseDelta()
    , KeyStates()
    , bAxisCacheDirty(true)
{
    if (FApplication::IsInitialized())
    {
        CursorInterface = FApplication::Get().GetCursor();
    }
}

void FPlayerInput::RebuildAxisCache()
{
    AxisValueCache.Clear();

    auto FindOrAddEntry = [this](const FInputName& AxisName) -> FAxisValueCacheEntry&
    {
        for (FAxisValueCacheEntry& Entry : AxisValueCache)
        {
            if (Entry.Name == AxisName)
            {
                return Entry;
            }
        }

        FAxisValueCacheEntry& NewEntry = AxisValueCache.Emplace();
        NewEntry.Name = AxisName;
        return NewEntry;
    };

    for (int32 Index = 0; Index < AxisMappings.Size(); ++Index)
    {
        FindOrAddEntry(AxisMappings[Index].Name).AxisMappingIndices.Add(Index);
    }

    for (int32 Index = 0; Index < AxisKeyMappings.Size(); ++Index)
    {
        FindOrAddEntry(AxisKeyMappings[Index].Name).AxisKeyMappingIndices.Add(Index);
    }

    bAxisCacheDirty = false;
}

void FPlayerInput::UpdateAxisValues()
{
    for (FAxisValueCacheEntry& Entry : AxisValueCache)
    {
        float AxisValue = 0.0f;

        for (int32 MappingIndex : Entry.AxisMappingIndices)
        {
            const FAxisMapping& AxisMapping = AxisMappings[MappingIndex];
            AxisValue += GetAnalogState(AxisMapping.Axis).Value * AxisMapping.Scale;
        }

        for (int32 MappingIndex : Entry.AxisKeyMappingIndices)
        {
            const FAxisKeyMapping& AxisKeyMapping = AxisKeyMappings[MappingIndex];
            if (IsKeyDown(AxisKeyMapping.Key))
            {
                AxisValue += AxisKeyMapping.Scale;
            }
        }

        Entry.Value = Math::Clamp(AxisValue, -1.0f, 1.0f);
    }
}

float FPlayerInput::GetAxisValueByHash(uint32 Hash) const
{
    for (const FAxisValueCacheEntry& Entry : AxisValueCache)
    {
        if (Entry.Name.GetHash() == Hash)
        {
            return Entry.Value;
        }
    }

    return 0.0f;
}

float FPlayerInput::GetAxisValue(const CHAR* AxisName) const
{
    return GetAxisValueByHash(FInputName(AxisName).GetHash());
}

void FPlayerInput::Tick(float DeltaTime)
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
            KeyState.TimePressed += DeltaTime;
        }

        // Check all the actions mappings if they should be invoked
        for (const FActionKeyMapping& KeyMapping : ActionKeyMappings)
        {
            if (KeyMapping.Key == KeyState.Key)
            {
                for (FInputComponent* InputComponent : ActiveInputComponents)
                {
                    for (const FActionInputBinding& ActionBinding : InputComponent->GetActionBindings())
                    {
                        if (ActionBinding.Name == KeyMapping.Name)
                        {
                            if (ActionBinding.ActionState == EActionState::Pressed)
                            {
                                if (KeyState.bIsDown && !KeyState.bPreviousState)
                                {
                                    InputActionsToCall.Add(ActionBinding.ActionDelegate);
                                }
                            }
                            else if (ActionBinding.ActionState == EActionState::Repeat)
                            {
                                if (KeyState.bIsDown && KeyState.bRepeatThisFrame)
                                {
                                    InputActionsToCall.Add(ActionBinding.ActionDelegate);
                                }
                            }
                            else if (ActionBinding.ActionState == EActionState::Released)
                            {
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

        KeyState.bRepeatThisFrame = 0;
        KeyState.bPreviousState   = KeyState.bIsDown;
        Index++;
    }

    // Precompute every distinct axis value once, rebuilding the grouping only when mappings changed
    if (bAxisCacheDirty)
    {
        RebuildAxisCache();
    }

    UpdateAxisValues();

    for (const FAxisValueCacheEntry& AxisEntry : AxisValueCache)
    {
        for (FInputComponent* InputComponent : ActiveInputComponents)
        {
            for (const FAxisInputBinding& AxisBinding : InputComponent->GetAxisBindings())
            {
                if (AxisBinding.Name == AxisEntry.Name)
                {
                    AxisBinding.ActionDelegate.ExecuteIfBound(AxisEntry.Value);
                }
            }
        }
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
    AxisStates.Clear();
    MouseDelta = IntVector2();
}

void FPlayerInput::ClearMouseDelta()
{
    MouseDelta = IntVector2();
}

void FPlayerInput::OnHighPrecisionMouseInput(const IntVector2& Delta)
{
    MouseDelta.X += Delta.X;
    MouseDelta.Y += Delta.Y;
}

IntVector2 FPlayerInput::ConsumeMouseDelta()
{
    const IntVector2 Result = MouseDelta;
    MouseDelta = IntVector2();
    return Result;
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
    bAxisCacheDirty = true;
    return MappingIndex;
}

int32 FPlayerInput::AddAxisKeyMapping(const FAxisKeyMapping& AxisKeyMapping)
{
    const int32 MappingIndex = AxisKeyMappings.Size();
    AxisKeyMappings.Add(AxisKeyMapping);
    bAxisCacheDirty = true;
    return MappingIndex;
}

void FPlayerInput::SetCursorPosition(const IntVector2& Position)
{
    if (TSharedPtr<IPlatformCursor> Cursor = GetCursorInterface())
    {
        Cursor->SetPosition(Position.X, Position.Y);
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

    const float DeadZone = AnalogInput::GetDeadzone(AxisSource);

    FAxisState& AxisState = AxisStates[Index];
    AxisState.Value = AnalogInput::ApplyDeadzone(AxisValue, DeadZone);
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
    KeyState.bPreviousState   = KeyState.bIsDown;
    KeyState.bIsDown          = bIsDown;
    KeyState.bRepeatThisFrame = bIsRepeat ? 1 : 0;

    if (bIsRepeat)
    {
        KeyState.RepeatCount++;
    }
    else
    {
        KeyState.RepeatCount = 0;
    }
}

IntVector2 FPlayerInput::GetCursorPosition() const
{
    if (TSharedPtr<IPlatformCursor> Cursor = GetCursorInterface())
    {
        Cursor->GetPosition();
    }

    return IntVector2(0, 0);
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
