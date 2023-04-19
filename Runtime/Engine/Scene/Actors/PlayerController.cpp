#include "PlayerController.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FPlayerInput::FPlayerInput()
    : Cursor(nullptr)
    , KeyStates()
    , MouseButtonStates()
{
}

void FPlayerInput::Tick(FTimespan Delta)
{
}

void FPlayerInput::ResetState()
{
}

void FPlayerInput::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
}

void FPlayerInput::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
}

void FPlayerInput::OnControllerButtonAnalog(const FControllerEvent& ControllerEvent)
{
}

void FPlayerInput::OnKeyUpEvent(const FKeyEvent& KeyEvent)
{
}

void FPlayerInput::OnKeyDownEvent(const FKeyEvent& KeyEvent)
{
}

void FPlayerInput::OnMouseButtonUpEvent(const FMouseEvent& MouseEvent)
{
}

void FPlayerInput::OnMouseButtonDownEvent(const FMouseEvent& MouseEvent)
{
}

void FPlayerInput::OnMouseMovedEvent(const FMouseEvent& MouseEvent)
{
}

void FPlayerInput::OnMouseScrolledEvent(const FMouseEvent& MouseEvent)
{
}

void FPlayerInput::SetCursorPosition(const FIntVector2& Postion)
{
}

FIntVector2 FPlayerInput::GetCursorPosition() const
{
    return FIntVector2{ 0, 0 };
}



FPlayerController::FPlayerController(FScene* InSceneOwner)
    : FActor(InSceneOwner)
    , InputComponent(nullptr)
    , PlayerInput(nullptr)
{
    FOBJECT_INIT();

    // Create a PlayerInput state object
    PlayerInput = new FPlayerInput();
}

FPlayerController::~FPlayerController()
{
    SAFE_DELETE(PlayerInput);
}

void FPlayerController::SetupInputComponent()
{
    if (!InputComponent)
    {
        InputComponent = new FInputComponent(this);
        AddComponent(InputComponent);
    }

    CHECK(InputComponent != nullptr);
}

void FPlayerController::Tick(FTimespan DeltaTime)
{
    // Call actor tick
    Super::Tick(DeltaTime);

    // Update the PlayerInput
    PlayerInput->Tick(DeltaTime);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
