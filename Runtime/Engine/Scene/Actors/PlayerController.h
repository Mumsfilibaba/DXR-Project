#pragma once
#include "Actor.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Input/InputStates.h"
#include "CoreApplication/Generic/ICursor.h"
#include "Application/Events.h"
#include "Engine/Scene/Components/InputComponent.h"

class ENGINE_API FPlayerInput
{
public:
    FPlayerInput();
    ~FPlayerInput() = default;

    void Tick(FTimespan Delta);

    void ResetState();

    virtual void OnKeyEvent(const FKeyEvent& KeyEvent);

    virtual void OnMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent);
    virtual void OnMouseMovedEvent(const FMouseMovedEvent& MouseMovedEvent);
    virtual void OnMouseScrolledEvent(const FMouseScrolledEvent& MouseScolledEvent);

    virtual void SetCursorPosition(const FIntVector2& Postion);
    virtual FIntVector2 GetCursorPosition() const;

public:
    FKeyState GetKeyState(EKey KeyCode) const
    {
        const int32 Index = GetKeyStateIndexFromKeyCode(KeyCode);
        return (Index < 0) ? FKeyState(KeyCode) : KeyStates[Index];
    }

    FMouseButtonState GetMouseButtonState(EMouseButton Button) const
    {
        const int32 Index = GetMouseButtonStateIndexFromMouseButton(Button);
        return (Index < 0) ? FMouseButtonState(Button) : MouseButtonStates[Index];
    }

    bool IsKeyDown(EKey KeyCode) const
    {
        const FKeyState KeyState = GetKeyState(KeyCode);
        return !!KeyState.IsDown;
    }

    bool IsKeyUp(EKey KeyCode) const
    {
        const FKeyState KeyState = GetKeyState(KeyCode);
        return !KeyState.IsDown;
    }

    bool IsKeyPressed(EKey KeyCode) const
    {
        const FKeyState KeyState = GetKeyState(KeyCode);
        return KeyState.IsDown && !KeyState.PreviousState;
    }

    bool IsButtonDown(EMouseButton Button) const
    {
        const FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !!ButtonState.IsDown;
    }

    bool IsButtonUp(EMouseButton Button) const
    {
        const FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !ButtonState.IsDown;
    }

    bool IsButtonPressed(EMouseButton Button) const
    {
        const FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return ButtonState.IsDown && !ButtonState.PreviousState;
    }

    TSharedPtr<ICursor> GetCursorInterface() const { return Cursor; }

private:
    FORCEINLINE int32 GetKeyStateIndexFromKeyCode(EKey KeyCode) const
    {
        FKeyState TmpState(KeyCode);
        return KeyStates.FindWithPredicate([&](const FKeyState& KeyState)
        {
            return (TmpState.KeyCode == KeyState.KeyCode);
        });
    }

    FORCEINLINE int32 GetMouseButtonStateIndexFromMouseButton(EMouseButton Button) const
    {
        FMouseButtonState TmpState(Button);
        return MouseButtonStates.FindWithPredicate([&](const FMouseButtonState& MouseState) -> bool
        {
            return (TmpState.Button == MouseState.Button);
        });
    }

    TSharedPtr<ICursor>       Cursor;

    TArray<FKeyState>         KeyStates;
    TArray<FMouseButtonState> MouseButtonStates;
};


class ENGINE_API FPlayerController
    : public FActor
{
    FOBJECT_BODY(FPlayerController, FActor);

public:
    FPlayerController(FScene* InSceneOwner);
    ~FPlayerController();

    virtual void SetupInputComponent();

    virtual void Tick(FTimespan DeltaTime) override;

    FInputComponent* GetInputComponent() const { return InputComponent; }
    FPlayerInput*    GetPlayerInput()    const { return PlayerInput; }

protected:
    FInputComponent* InputComponent;
    FPlayerInput*    PlayerInput;
};