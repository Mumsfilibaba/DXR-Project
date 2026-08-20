#pragma once
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/PlatformInterface/IPlatformCursor.h"
#include "CoreApplication/PlatformInterface/InputCodes.h"
#include "Application/Events.h"
#include "Application/Input/Keys.h"
#include "Engine/World/Actors/InputName.h"

class FInputComponent;

struct FKeyState
{
    FKeyState(FKey InKey)
        : Key(InKey)
        , bIsDown(0)
        , bPreviousState(0)
        , RepeatCount(0)
        , bRepeatThisFrame(0)
        , TimePressed(0.0f)
    {
    }

    /** @brief The key for this key-state */
    const FKey Key;
    
    /** @brief Indicates if the current state of the key is down */
    uint32 bIsDown : 1;
    
    /** @brief Indicates if the last state of the key was down (1) or if it was up (0) */
    uint32 bPreviousState : 1;
    
    /** @brief If this state is repeated due to the user holding down the key, the repeat-count is increased */
    uint32 RepeatCount : 30;

    /** @brief True when the most recent key event for this key was an OS repeat */
    uint32 bRepeatThisFrame : 1;

    /** @brief Time pressed */
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

    /** @brief The analog source that this came from */
    const EAnalogSourceName::Type Source;

    /** @brief The current analog value */
    float Value;

    /** @brief Amount of frames that has passed since the value was updated */
    int32 NumTicksSinceUpdate;
};

struct FActionKeyMapping
{
    FActionKeyMapping() = default;

    FActionKeyMapping(const FInputName& InName, FKey InKey)
        : Name(InName)
        , Key(InKey)
    {
    }

    FInputName Name;
    FKey       Key;
};

struct FAxisMapping
{
    FAxisMapping()
        : Scale(1.0f)
    {
    }

    FAxisMapping(const FInputName& InName, EAnalogSourceName::Type InAxis, float InScale = 1.0f)
        : Name(InName)
        , Axis(InAxis)
        , Scale(InScale)
    {
    }

    FInputName              Name;
    EAnalogSourceName::Type Axis;
    float                   Scale;
};

struct FAxisKeyMapping
{
    FAxisKeyMapping() = default;

    FAxisKeyMapping(const FInputName& InName, FKey InKey, float InScale)
        : Name(InName)
        , Key(InKey)
        , Scale(InScale)
    {
    }

    FInputName Name;
    FKey       Key;
    float      Scale;
};

class ENGINE_API FPlayerInput
{
public:
    FPlayerInput();
    ~FPlayerInput() = default;

    void Tick(float DeltaTime);
    
    void EnableInput(FInputComponent* InputComponent);
    void ClearInputStates();
    void ClearMouseDelta();

    IntVector2 ConsumeMouseDelta();

    int32 AddActionKeyMapping(const FActionKeyMapping& ActionKeyMapping);
    int32 AddAxisMapping(const FAxisMapping& AxisMapping);
    int32 AddAxisKeyMapping(const FAxisKeyMapping& AxisKeyMapping);

    void OnAxisEvent(EAnalogSourceName::Type AxisSource, float AxisValue);
    void OnKeyEvent(FKey Key, bool bIsDown, bool bIsRepeat);
    void OnHighPrecisionMouseInput(const IntVector2& Delta);

    void SetCursorPosition(const IntVector2& Postion);

    FKeyState  GetKeyState(FKey Key) const;
    IntVector2 GetCursorPosition() const;
    FAxisState GetAnalogState(EAnalogSourceName::Type AnalogSource) const;
    float      GetAxisValue(const CHAR* AxisName) const;

    template<typename StringType>
    FORCEINLINE float GetAxisValue(const StringType& AxisName) const requires(TIsTStringType<StringType>::Value)
    {
        return GetAxisValueByHash(FInputName::HashInputName(AxisName));
    }

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

    TSharedPtr<IPlatformCursor> GetCursorInterface() const 
    { 
        return CursorInterface;
    }

private:

    struct FAxisValueCacheEntry
    {
        FInputName    Name;
        TArray<int32> AxisMappingIndices;
        TArray<int32> AxisKeyMappingIndices;
        float         Value = 0.0f;
    };

    void  ClearEvents();
    void  RebuildAxisCache();
    void  UpdateAxisValues();
    float GetAxisValueByHash(uint32 Hash) const;

    TSharedPtr<IPlatformCursor>  CursorInterface;
    IntVector2                   MouseDelta;
    TArray<FKeyState>            KeyStates;
    TArray<FAxisState>           AxisStates;
    TArray<FActionKeyMapping>    ActionKeyMappings;
    TArray<FAxisMapping>         AxisMappings;
    TArray<FAxisKeyMapping>      AxisKeyMappings;
    TArray<FInputComponent*>     ActiveInputComponents;
    TArray<FAxisValueCacheEntry> AxisValueCache;
    bool                         bAxisCacheDirty;
};
