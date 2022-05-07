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
// CCanvasUser

class CANVAS_API CCanvasUser
{
    friend class CCanvasApplication;

public:

    static FORCEINLINE TSharedPtr<CCanvasUser> Make(uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor)
    {
        return TSharedPtr<CCanvasUser>(dbg_new CCanvasUser(InUserIndex, InCursor));
    }

    virtual ~CCanvasUser() = default;

    virtual void Tick(CTimestamp DeltaTime);

    virtual void HandleKeyEvent(const SKeyEvent& KeyEvent);

    virtual void HandleMouseButtonEvent(const SMouseButtonEvent& MouseButtonEvent);

    virtual void HandleMouseMovedEvent(const SMouseMovedEvent& MouseMovedEvent);

    virtual void HandleMouseScrolledEvent(const SMouseScrolledEvent& MouseScolledEvent);

    virtual void SetCursorPosition(const CIntVector2& Postion);

    virtual CIntVector2 GetCursorPosition() const;

    FORCEINLINE SKeyState GetKeyState(EKey KeyCode) const
    {
        int32 Index = GetKeyStateIndexFromKeyCode(KeyCode);
        if (Index < 0)
        {
            return SKeyState(KeyCode);
        }
        else
        {
            return KeyStates[Index];
        }
    }

    FORCEINLINE SMouseButtonState GetMouseButtonState(EMouseButton Button) const
    {
        int32 Index = GetMouseButtonStateIndexFromMouseButton(Button);
        if (Index < 0)
        {
            return SMouseButtonState(Button);
        }
        else
        {
            return MouseButtonStates[Index];
        }
    }

    FORCEINLINE bool IsKeyDown(EKey KeyCode) const
    {
        SKeyState KeyState = GetKeyState(KeyCode);
        return !!KeyState.IsDown;
    }

    FORCEINLINE bool IsKeyUp(EKey KeyCode) const
    {
        SKeyState KeyState = GetKeyState(KeyCode);
        return !KeyState.IsDown;
    }

    FORCEINLINE bool IsKeyPressed(EKey KeyCode) const
    {
        SKeyState KeyState = GetKeyState(KeyCode);
        return KeyState.IsDown && !KeyState.PreviousState;
    }

    FORCEINLINE bool IsMouseButtonDown(EMouseButton Button) const
    {
        SMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !!ButtonState.IsDown;
    }

    FORCEINLINE bool IsMouseButtonUp(EMouseButton Button) const
    {
        SMouseButtonState ButtonState = GetMouseButtonState(Button);
        return !ButtonState.IsDown;
    }

    FORCEINLINE bool IsMouseButtonPressed(EMouseButton Button) const
    {
        SMouseButtonState ButtonState = GetMouseButtonState(Button);
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

    CCanvasUser(uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor);

     /** @brief: Get the index in the key-state array */
    FORCEINLINE int32 GetKeyStateIndexFromKeyCode(EKey KeyCode) const
    {
        SKeyState TempState(KeyCode);
        return KeyStates.Find(TempState, [](const SKeyState& LHS, const SKeyState& RHS) -> bool
        {
            return (LHS.KeyCode == RHS.KeyCode);
        });
    }

     /** @brief: Get the index in the key-state array */
    FORCEINLINE int32 GetMouseButtonStateIndexFromMouseButton(EMouseButton Button) const
    {
        SMouseButtonState TempState(Button);
        return MouseButtonStates.Find(TempState, [](const SMouseButtonState& LHS, const SMouseButtonState& RHS) -> bool
        {
            return (LHS.Button == RHS.Button);
        });
    }

    const uint32              UserIndex;
    
    TSharedPtr<ICursor>       Cursor;

    TArray<SKeyState>         KeyStates; // TODO: Use a map instead? 
    TArray<SMouseButtonState> MouseButtonStates;
};
