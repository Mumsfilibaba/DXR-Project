#pragma once
#include "Events.h"
#include "Canvas.h"

#include "Core/Input/InputStates.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Time/Timestamp.h"
#include "Core/Math/IntVector2.h"

#include "CoreApplication/ICursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FUser

class CANVAS_API FUser
{
    friend class FApplication;

public:

    static FORCEINLINE TSharedPtr<FUser> Make(uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor)
    {
        return TSharedPtr<FUser>(dbg_new FUser(InUserIndex, InCursor));
    }

    virtual ~FUser() = default;

    virtual void Tick(FTimestamp DeltaTime);

    virtual void HandleKeyEvent(const FKeyEvent& KeyEvent);

    virtual void HandleMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent);

    virtual void HandleMouseMovedEvent(const FMouseMovedEvent& MouseMovedEvent);

    virtual void HandleMouseScrolledEvent(const FMouseScrolledEvent& MouseScolledEvent);

    virtual void SetCursorPosition(const FIntVector2& Postion);

    virtual FIntVector2 GetCursorPosition() const;

    FORCEINLINE FKeyState GetKeyState(EKey KeyCode) const
    {
        int32 Index = GetKeyStateIndexFromKeyCode(KeyCode);
        if (Index < 0)
        {
            return FKeyState(KeyCode);
        }
        else
        {
            return KeyStates[Index];
        }
    }

    FORCEINLINE FMouseButtonState GetMouseButtonState(EMouseButton Button) const
    {
        int32 Index = GetMouseButtonStateIndexFromMouseButton(Button);
        if (Index < 0)
        {
            return FMouseButtonState(Button);
        }
        else
        {
            return MouseButtonStates[Index];
        }
    }

    FORCEINLINE bool IsKeyDown(EKey KeyCode) const
    {
        FKeyState KeyState = GetKeyState(KeyCode);
        return !!KeyState.IsDown;
    }

    FORCEINLINE bool IsKeyUp(EKey KeyCode) const
    {
        FKeyState KeyState = GetKeyState(KeyCode);
        return !KeyState.IsDown;
    }

    FORCEINLINE bool IsKeyPressed(EKey KeyCode) const
    {
        FKeyState KeyState = GetKeyState(KeyCode);
        return KeyState.IsDown && !KeyState.PreviousState;
    }

    FORCEINLINE bool IsMouseButtonDown(EMouseButton Button) const
    {
        FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !!ButtonState.IsDown;
    }

    FORCEINLINE bool IsMouseButtonUp(EMouseButton Button) const
    {
        FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !ButtonState.IsDown;
    }

    FORCEINLINE bool IsMouseButtonPressed(EMouseButton Button) const
    {
        FMouseButtonState ButtonState = GetMouseButtonState(Button);
        return ButtonState.IsDown && !ButtonState.PreviousState;
    }

    FORCEINLINE TSharedPtr<ICursor> GetCursorDevice() const
    {
        return Cursor;
    }

    FORCEINLINE uint32 GetUserIndex() const
    {
        return UserIndex;
    }

private:
    FUser(uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor);

     /** @brief: Get the index in the key-state array */
    FORCEINLINE int32 GetKeyStateIndexFromKeyCode(EKey KeyCode) const
    {
        FKeyState TempState(KeyCode);
        return KeyStates.Find(TempState, [](const FKeyState& LHS, const FKeyState& RHS) -> bool
        {
            return (LHS.KeyCode == RHS.KeyCode);
        });
    }

     /** @brief: Get the index in the key-state array */
    FORCEINLINE int32 GetMouseButtonStateIndexFromMouseButton(EMouseButton Button) const
    {
        FMouseButtonState TempState(Button);
        return MouseButtonStates.Find(TempState, [](const FMouseButtonState& LHS, const FMouseButtonState& RHS) -> bool
        {
            return (LHS.Button == RHS.Button);
        });
    }

    const uint32              UserIndex;
    
    TSharedPtr<ICursor>       Cursor;

    TArray<FKeyState>         KeyStates; // TODO: Use a map instead? 
    TArray<FMouseButtonState> MouseButtonStates;
};
