#pragma once
#include "Actor.h"
#include "Application/Events.h"
#include "Core/Math/IntVector2.h"
#include "Core/Input/InputStates.h"
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

    virtual void OnControllerEvent(const FControllerEvent& ControllerEvent);
    
    virtual void OnKeyEvent(const FKeyEvent& KeyEvent);
    
    virtual void OnMouseEvent(const FMouseEvent& MouseEvent);

    FKeyState              GetKeyState(EKeyName::Type Key) const;

    FMouseButtonState      GetMouseButtonState(EMouseButtonName Button) const;
    
    FControllerButtonState GetControllerButtonState(EGamepadButtonName Button) const;
    
    FAnalogAxisState       GetAnalogState(EAnalogSourceName AnalogSource) const;

    void ResetStates();


    bool IsKeyDown(EKeyName::Type Key) const
    {
        const FKeyState KeyState = GetKeyState(Key);
        return !!KeyState.bIsDown;
    }

    bool IsKeyUp(EKeyName::Type Key) const
    {
        const FKeyState KeyState = GetKeyState(Key);
        return !KeyState.bIsDown;
    }

    bool IsKeyPressed(EKeyName::Type Key) const
    {
        const FKeyState KeyState = GetKeyState(Key);
        return !!KeyState.bIsDown && !KeyState.bPreviousState;
    }


    bool IsButtonDown(EMouseButtonName Button) const
    {
        const FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !!ButtonState.bIsDown;
    }

    bool IsButtonUp(EMouseButtonName Button) const
    {
        const FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !ButtonState.bIsDown;
    }

    bool IsButtonPressed(EMouseButtonName Button) const
    {
        const FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !!ButtonState.bIsDown && !ButtonState.bPreviousState;
    }


    bool IsButtonDown(EGamepadButtonName Button) const
    {
        const FControllerButtonState ButtonState = GetControllerButtonState(Button);
        return !!ButtonState.bIsDown;
    }

    bool IsButtonUp(EGamepadButtonName Button) const
    {
        const FControllerButtonState ButtonState = GetControllerButtonState(Button);
        return !ButtonState.bIsDown;
    }

    bool IsButtonPressed(EGamepadButtonName Button) const
    {
        const FControllerButtonState ButtonState = GetControllerButtonState(Button);
        return !!ButtonState.bIsDown && !ButtonState.bPreviousState;
    }


    TSharedPtr<ICursor> GetCursorInterface() const 
    { 
        return CursorInterface;
    }

private:
    void ClearEvents();

    TArray<FKeyState>              KeyStates;
    TArray<FMouseButtonState>      MouseButtonStates;
    TArray<FControllerButtonState> ControllerButtonStates;
    TArray<FAnalogAxisState>       AnalogAxisStates;

    TArray<FKeyEvent>         KeyEvents;
    TArray<FMouseEvent>       MouseEvents;
    TArray<FControllerEvent>  ControllerEvents;

    TSharedPtr<ICursor>       CursorInterface;
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