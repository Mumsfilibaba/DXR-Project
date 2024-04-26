#pragma once
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/ICursor.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "Application/Events.h"
#include "Application/Input/Keys.h"

struct FKeyState
{
    FKeyState(FKey InKey)
        : Key(InKey)
        , bIsDown(0)
        , bPreviousState(0)
        , RepeatCount(0)
        , TimePressed(0.0f)
    {
    }

    /** @brief - The key for this key-state */
    const FKey Key;
    
    /** @brief - Indicates if the current state of the key is down */
    uint32 bIsDown : 1;
    
    /** @brief - Indicates if the last state of the key was down (1) or if it was up (0) */
    uint32 bPreviousState : 1;
    
    /** @brief - If this state is repeated due to the user holding down the key, the repeat-count is increased */
    uint32 RepeatCount : 30;

    /** @brief - Time pressed */
    float TimePressed = 0.0f;
};

struct FAxisState
{
    FAxisState(EAnalogSourceName::Type InSource)
        : Source(InSource)
        , Value(0.0f)
        , NumTicksSinceUpdate(0)
    {
    }

    /** @brief - The analog source that this came from */
    const EAnalogSourceName::Type Source;

    /** @brief - The current analog value */
    float Value;

    /** @brief - Amount of frames that has passed since the value was updated */
    int32 NumTicksSinceUpdate;
};

struct FActionKeyMapping
{
    FActionKeyMapping() = default;

    FActionKeyMapping(const FStringView& InName, FKey InKey)
        : Name(InName)
        , Key(InKey)
    {
    }

    FString Name;
    FKey    Key;
};

struct FAxisMapping
{
    FAxisMapping() = default;

    FAxisMapping(const FStringView& InName, EAnalogSourceName::Type InAxis)
        : Name(InName)
        , Axis(InAxis)
    {
    }

    FString                 Name;
    EAnalogSourceName::Type Axis;
};

struct FAxisKeyMapping
{
    FAxisKeyMapping() = default;

    FAxisKeyMapping(const FStringView& InName, FKey InKey, float InScale)
        : Name(InName)
        , Key(InKey)
        , Scale(InScale)
    {
    }

    FString Name;
    FKey    Key;
    float   Scale;
};

class FInputComponent;

class ENGINE_API FPlayerInput
{
public:
    FPlayerInput();
    ~FPlayerInput() = default;

    void Tick(FTimespan Delta);
    void EnableInput(FInputComponent* InputComponent);
    void ClearInputStates();
    int32 AddActionKeyMapping(const FActionKeyMapping& ActionKeyMapping);
    int32 AddAxisMapping(const FAxisMapping& AxisMapping);
    int32 AddAxisKeyMapping(const FAxisKeyMapping& AxisKeyMapping);
    void SetCursorPosition(const FIntVector2& Postion);
    void OnAxisEvent(EAnalogSourceName::Type AxisSource, float AxisValue);
    void OnKeyEvent(FKey Key, bool bIsDown, bool bIsRepeat);

    bool IsKeyDown(FKey Key) const
    {
        const FKeyState KeyState = GetKeyState(Key);
        return KeyState.bIsDown;
    }

    bool IsKeyUp(FKey Key) const
    {
        const FKeyState KeyState = GetKeyState(Key);
        return !KeyState.bIsDown;
    }

    bool WasKeyPressed(FKey Key) const
    {
        const FKeyState KeyState = GetKeyState(Key);
        return KeyState.bIsDown && !KeyState.bPreviousState;
    }

    FIntVector2 GetCursorPosition() const;
    FKeyState GetKeyState(FKey Key) const;
    FAxisState GetAnalogState(EAnalogSourceName::Type AnalogSource) const;

    TSharedPtr<ICursor> GetCursorInterface() const 
    { 
        return CursorInterface;
    }

private:
    void ClearEvents();

    TSharedPtr<ICursor>       CursorInterface;

    TArray<FKeyState>         KeyStates;
    TArray<FAxisState>        AxisStates;
    
    TArray<FActionKeyMapping> ActionKeyMappings;
    TArray<FAxisMapping>      AxisMappings;
    TArray<FAxisKeyMapping>   AxisKeyMappings;
    
    TArray<FInputComponent*>  ActiveInputComponents;
};
