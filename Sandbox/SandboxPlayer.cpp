#include "SandboxPlayer.h"
#include <Engine/Scene/Camera.h>
#include <Engine/Scene/Scene.h>

FSandboxPlayerController::FSandboxPlayerController(FScene* InScene)
    : FPlayerController(InScene)
    , Camera(nullptr)
{
    Camera = new FCamera();
    GetScene()->AddCamera(Camera);
}

void FSandboxPlayerController::Tick(FTimespan DeltaTime)
{
    Super::Tick(DeltaTime);

    const float Delta = static_cast<float>(DeltaTime.AsSeconds());
    const float RotationSpeed = 45.0f;

    FPlayerInput* InputState = GetPlayerInput();
    if (InputState->IsKeyDown(EKey::Key_Right))
    {
        Camera->Rotate(0.0f, NMath::ToRadians(RotationSpeed * Delta), 0.0f);
    }
    else if (InputState->IsKeyDown(EKey::Key_Left))
    {
        Camera->Rotate(0.0f, NMath::ToRadians(-RotationSpeed * Delta), 0.0f);
    }

    if (InputState->IsKeyDown(EKey::Key_Up))
    {
        Camera->Rotate(NMath::ToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
    }
    else if (InputState->IsKeyDown(EKey::Key_Down))
    {
        Camera->Rotate(NMath::ToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
    }

    float Acceleration = 15.0f;
    if (InputState->IsKeyDown(EKey::Key_LeftShift))
    {
        Acceleration = Acceleration * 3;
    }

    FVector3 CameraAcceleration;
    if (InputState->IsKeyDown(EKey::Key_W))
    {
        CameraAcceleration.z = Acceleration;
    }
    else if (InputState->IsKeyDown(EKey::Key_S))
    {
        CameraAcceleration.z = -Acceleration;
    }

    if (InputState->IsKeyDown(EKey::Key_A))
    {
        CameraAcceleration.x = Acceleration;
    }
    else if (InputState->IsKeyDown(EKey::Key_D))
    {
        CameraAcceleration.x = -Acceleration;
    }

    if (InputState->IsKeyDown(EKey::Key_Q))
    {
        CameraAcceleration.y = Acceleration;
    }
    else if (InputState->IsKeyDown(EKey::Key_E))
    {
        CameraAcceleration.y = -Acceleration;
    }

    const float Deacceleration = -5.0f;
    CameraSpeed = CameraSpeed + (CameraSpeed * Deacceleration) * Delta;
    CameraSpeed = CameraSpeed + (CameraAcceleration * Delta);

    const FVector3 Speed = CameraSpeed * Delta;
    Camera->Move(Speed.x, Speed.y, Speed.z);
    Camera->UpdateMatrices();
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
