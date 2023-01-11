#include "PlayerController.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

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


FPlayerInput::FPlayerInput()
    : Cursor(nullptr)
    , KeyStates()
    , MouseButtonStates()
{ }

void FPlayerInput::Tick(FTimespan Delta)
{
}

void FPlayerInput::OnKeyEvent(const FKeyEvent& KeyEvent)
{
}

void FPlayerInput::OnCursorButtonEvent(const FMouseButtonEvent& MouseButtonEvent)
{
}

void FPlayerInput::OnCursorMovedEvent(const FMouseMovedEvent& MouseMovedEvent)
{
}

void FPlayerInput::OnCursorScrolledEvent(const FMouseScrolledEvent& MouseScolledEvent)
{
}

void FPlayerInput::SetCursorPosition(const FIntVector2& Postion)
{
}

FIntVector2 FPlayerInput::GetCursorPosition() const
{
    return FIntVector2{ 0, 0 };
}

void FPlayerInput::ResetState()
{
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
