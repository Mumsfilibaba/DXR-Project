#include "SandboxPlayer.h"

#include <Engine/Scene/Camera.h>
#include <Engine/Scene/Scene.h>

FSandboxPlayerController::FSandboxPlayerController(FScene* InScene)
    : FPlayerController(InScene)
    , Camera(nullptr)
{
    Camera = new FCamera();
    InScene->AddCamera(Camera);
}

void FSandboxPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindAction("Forward", EActionState::Pressed, this);
        InputComponent->BindAction("Backward", EActionState::Pressed, this);
        InputComponent->BindAction("Right", EActionState::Pressed, this);
        InputComponent->BindAction("Left", EActionState::Pressed, this);

        InputComponent->BindAction("RotateLeft", EActionState::Pressed, this);
        InputComponent->BindAction("RotateRight", EActionState::Pressed, this);
    }
}

void FSandboxPlayerController::MoveForward()
{
}

void FSandboxPlayerController::MoveBackward()
{
}

void FSandboxPlayerController::MoveRight()
{
}

void FSandboxPlayerController::MoveLeft()
{
}
