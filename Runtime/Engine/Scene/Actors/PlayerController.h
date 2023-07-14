#pragma once
#include "Actor.h"
#include "InputStates.h"
#include "Application/Events.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Set.h"
#include "CoreApplication/Generic/ICursor.h"
#include "Engine/Scene/Components/InputComponent.h"

class ENGINE_API FPlayerInput
{
public:
    FPlayerInput();
    virtual ~FPlayerInput() = default;

    void Tick(FTimespan Delta);

    virtual void SetCursorPosition(const FIntVector2& Postion);

    virtual FIntVector2 GetCursorPosition() const;

    virtual void OnAnalogGamepadEvent(const FAnalogGamepadEvent& AnalogGamepadEvent);
    
    virtual void OnKeyEvent(const FKeyEvent& KeyEvent);
    
    virtual void OnCursorEvent(const FCursorEvent& MouseEvent);

    FKeyState GetKeyState(FKey Key) const;
    
    FAnalogAxisState GetAnalogState(EAnalogSourceName::Type AnalogSource) const;

    void ClearInputStates();

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

    TSharedPtr<ICursor> GetCursorInterface() const 
    { 
        return CursorInterface;
    }

private:
    void ClearEvents();

    TSharedPtr<ICursor>         CursorInterface;

    TArray<FKeyState>           KeyStates;
    TArray<FAnalogAxisState>    AnalogAxisStates;

    TArray<FCursorEvent>        MouseEvents;
    TArray<FKeyEvent>           KeyEvents;
    TArray<FAnalogGamepadEvent> ControllerEvents;
};


class ENGINE_API FPlayerController : public FActor
{
    FOBJECT_BODY(FPlayerController, FActor);

public:
    FPlayerController(FScene* InSceneOwner);
    ~FPlayerController();

    virtual void SetupInputComponent();

    virtual void Tick(FTimespan DeltaTime) override;

    FInputComponent* GetInputComponent() const
    {
        return InputComponent;
    }

    FPlayerInput* GetPlayerInput() const
    {
        return PlayerInput;
    }

protected:
    FInputComponent* InputComponent;
    FPlayerInput*    PlayerInput;
};
