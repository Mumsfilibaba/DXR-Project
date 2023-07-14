#include "SandboxPlayer.h"
#include <Engine/Scene/Camera.h>
#include <Engine/Scene/Scene.h>
#include <Core/Misc/OutputDeviceLogger.h>

FSandboxPlayerController::FSandboxPlayerController(FScene* InScene)
    : FPlayerController(InScene)
    , Camera(nullptr)
{
    Camera = new FCamera();
    Camera->Move(0.0f, 10.0f, -2.0f);

    GetScene()->AddCamera(Camera);
}

void FSandboxPlayerController::Tick(FTimespan DeltaTime)
{
    Super::Tick(DeltaTime);

    const float Delta         = static_cast<float>(DeltaTime.AsSeconds());
    const float RotationSpeed = 45.0f;
    const float Deadzone      = 0.01f;

    FPlayerInput* InputState = GetPlayerInput();
    CHECK(InputState != nullptr);
    
    const FAnalogAxisState RightThumbX = InputState->GetAnalogState(EAnalogSourceName::RightThumbX);
    const FAnalogAxisState RightThumbY = InputState->GetAnalogState(EAnalogSourceName::RightThumbY);
    
    if (FMath::Abs(RightThumbX.Value) > Deadzone)
    {
        Camera->Rotate(0.0f, FMath::ToRadians(RightThumbX.Value * RotationSpeed * Delta), 0.0f);
    }
    else if (InputState->IsKeyDown(EKeys::Right))
    {
        Camera->Rotate(0.0f, FMath::ToRadians(RotationSpeed * Delta), 0.0f);
    }
    else if (InputState->IsKeyDown(EKeys::Left))
    {
        Camera->Rotate(0.0f, FMath::ToRadians(-RotationSpeed * Delta), 0.0f);
    }

    if (FMath::Abs(RightThumbY.Value) > Deadzone)
    {
        Camera->Rotate(FMath::ToRadians(-RightThumbY.Value * RotationSpeed * Delta), 0.0f, 0.0f);
    }
    else if (InputState->IsKeyDown(EKeys::Up))
    {
        Camera->Rotate(FMath::ToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
    }
    else if (InputState->IsKeyDown(EKeys::Down))
    {
        Camera->Rotate(FMath::ToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
    }

    float Acceleration = 15.0f;
    if (InputState->IsKeyDown(EKeys::LeftShift) || InputState->IsKeyDown(EKeys::GamepadLeftTrigger))
    {
        Acceleration = Acceleration * 3;
    }

    const FAnalogAxisState LeftThumbX = InputState->GetAnalogState(EAnalogSourceName::LeftThumbX);
    const FAnalogAxisState LeftThumbY = InputState->GetAnalogState(EAnalogSourceName::LeftThumbY);

    FVector3 CameraAcceleration;
    if (FMath::Abs(LeftThumbY.Value) > Deadzone)
    {
        CameraAcceleration.z = Acceleration * LeftThumbY.Value;
    }
    else if (InputState->IsKeyDown(EKeys::W))
    {
        CameraAcceleration.z = Acceleration;
    }
    else if (InputState->IsKeyDown(EKeys::S))
    {
        CameraAcceleration.z = -Acceleration;
    }

    if (FMath::Abs(LeftThumbX.Value) > Deadzone)
    {
        CameraAcceleration.x = Acceleration * -LeftThumbX.Value;
    }
    else if (InputState->IsKeyDown(EKeys::A))
    {
        CameraAcceleration.x = Acceleration;
    }
    else if (InputState->IsKeyDown(EKeys::D))
    {
        CameraAcceleration.x = -Acceleration;
    }

    if (InputState->IsKeyDown(EKeys::Q))
    {
        CameraAcceleration.y = Acceleration;
    }
    else if (InputState->IsKeyDown(EKeys::E))
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
