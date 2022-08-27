#include "User.h"
#include "ApplicationInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FUser

FUser::FUser(uint32 InUserIndex, const TSharedPtr<ICursor>& InCursor)
    : UserIndex(InUserIndex)
    , Cursor(InCursor)
    , KeyStates()
    , MouseButtonStates()
{ }

void FUser::Tick(FTimespan DeltaTime)
{
    // Update all key-states 
    for (FKeyState& KeyState : KeyStates)
    {
        if (KeyState.IsDown)
        {
            KeyState.TimePressed += (float)DeltaTime.AsMilliseconds();
        }
    }

    // Update all mouse-button states
    for (FMouseButtonState& MouseButtonState : MouseButtonStates)
    {
        if (MouseButtonState.IsDown)
        {
            MouseButtonState.TimePressed += (float)DeltaTime.AsMilliseconds();
        }
    }
}

void FUser::HandleKeyEvent(const FKeyEvent& KeyEvent)
{
    int32 Index = GetKeyStateIndexFromKeyCode(KeyEvent.KeyCode);
    if (Index >= 0)
    {
        FKeyState& KeyState = KeyStates[Index];
        if (KeyEvent.bIsDown)
        {
            KeyState.PreviousState = KeyState.IsDown;
            KeyState.IsDown = KeyEvent.bIsDown;

            if (KeyEvent.bIsRepeat)
            {
                KeyState.RepeatCount++;
            }
        }
        else
        {
            KeyState.IsDown = 0;
            KeyState.PreviousState = 0;
            KeyState.RepeatCount = 0;
            KeyState.TimePressed = 0.0f;
        }
    }
    else
    {
        FKeyState NewKeyState(KeyEvent.KeyCode);
        NewKeyState.IsDown = KeyEvent.bIsDown;
        KeyStates.Push(NewKeyState);
    }
}

void FUser::HandleMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent)
{
    int32 Index = GetMouseButtonStateIndexFromMouseButton(MouseButtonEvent.Button);
    if (Index >= 0)
    {
        FMouseButtonState& MouseButtonState = MouseButtonStates[Index];
        if (MouseButtonState.IsDown)
        {
            MouseButtonState.PreviousState = MouseButtonState.IsDown;
            MouseButtonState.IsDown = MouseButtonEvent.bIsDown;
        }
        else
        {
            MouseButtonState.IsDown = 0;
            MouseButtonState.PreviousState = 0;
            MouseButtonState.TimePressed = 0.0f;
        }
    }
    else
    {
        FMouseButtonState NewMouseButtonState(MouseButtonEvent.Button);
        NewMouseButtonState.IsDown = MouseButtonEvent.bIsDown;
        MouseButtonStates.Push(NewMouseButtonState);
    }
}

void FUser::HandleMouseMovedEvent(const FMouseMovedEvent& MouseMovedEvent)
{
    UNREFERENCED_VARIABLE(MouseMovedEvent);
    // TODO: Call all attached player controllers 
}

void FUser::HandleMouseScrolledEvent(const FMouseScrolledEvent& MouseScolledEvent)
{
    UNREFERENCED_VARIABLE(MouseScolledEvent);
    // TODO: Call all attached player controllers
}

void FUser::SetCursorPosition(const FIntVector2& Postion)
{
    if (Cursor)
    {
        // TODO: Relative window
        Cursor->SetPosition(nullptr, Postion.x, Postion.y);
    }
}

FIntVector2 FUser::GetCursorPosition() const
{
    if (Cursor)
    {
        FIntVector2 Position;
        Cursor->GetPosition(nullptr, Position.x, Position.y);
        return Position;
    }
    else
    {
        return FIntVector2(-1, -1);
    }
}
