#pragma once
#include "Events.h"
#include "ApplicationImpl.h"

#include "Core/Input/InputStates.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"

#include "CoreApplication/ICursor.h"

class APPLICATION_API FUser
{
    friend class FApplication;

public:

    static FORCEINLINE TSharedPtr<FUser> Make(uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor)
    {
        return TSharedPtr<FUser>(new FUser(InUserIndex, InCursor));
    }

    virtual ~FUser() = default;

    virtual void Tick(FTimespan DeltaTime);

    virtual void HandleKeyEvent(const FKeyEvent& KeyEvent);

    virtual void HandleMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent);

    virtual void HandleMouseMovedEvent(const FMouseMovedEvent& MouseMovedEvent);

    virtual void HandleMouseScrolledEvent(const FMouseScrolledEvent& MouseScolledEvent);

    virtual void SetCursorPosition(const FIntVector2& Postion);

    virtual FIntVector2 GetCursorPosition() const;

    void ResetState();

    FORCEINLINE FKeyState GetKeyState(EKey KeyCode) const
    {
        const int32 Index = GetKeyStateIndexFromKeyCode(KeyCode);
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
        const int32 Index = GetMouseButtonStateIndexFromMouseButton(Button);
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

    /** @brief - Get the index in the key-state array */
    FORCEINLINE int32 GetKeyStateIndexFromKeyCode(EKey KeyCode) const
    {
        FKeyState TmpState(KeyCode);
        return KeyStates.FindWithPredicate([&](const FKeyState& KeyState)
        {
            return (TmpState.KeyCode == KeyState.KeyCode);
        });
    }

    /** @brief - Get the index in the key-state array */
    FORCEINLINE int32 GetMouseButtonStateIndexFromMouseButton(EMouseButton Button) const
    {
        FMouseButtonState TmpState(Button);
        return MouseButtonStates.FindWithPredicate([&](const FMouseButtonState& MouseState) -> bool
        {
            return (TmpState.Button == MouseState.Button);
        });
    }

    const uint32              UserIndex;
    
    TSharedPtr<ICursor>       Cursor;

    TArray<FKeyState>         KeyStates;
    TArray<FMouseButtonState> MouseButtonStates;
};
