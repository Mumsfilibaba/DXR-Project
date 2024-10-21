#include "SandboxPlayer.h"
#include <Core/Misc/OutputDeviceLogger.h>
#include <Engine/World/Camera.h>
#include <Engine/World/World.h>
#include <Engine/World/Actors/PlayerInput.h>
#include <Engine/World/Components/InputComponent.h>

FOBJECT_IMPLEMENT_CLASS(FSandboxPlayerController);

FSandboxPlayerController::FSandboxPlayerController(const FObjectInitializer& Initializer)
    : FPlayerController(Initializer)
    , Camera(nullptr)
{
    Camera = new FCamera();
    Camera->Move(0.0f, 10.0f, -2.0f);

    // Bind input mappings
    if (FPlayerInput* Input = GetPlayerInput())
    {
        FActionKeyMapping MoveForwardKeyMapping("MoveForward", EKeys::W);
        Input->AddActionKeyMapping(MoveForwardKeyMapping);

        FActionKeyMapping MoveBackwardsKeyMapping("MoveBackwards", EKeys::S);
        Input->AddActionKeyMapping(MoveBackwardsKeyMapping);

        FActionKeyMapping MoveLeftKeyMapping("MoveLeft", EKeys::A);
        Input->AddActionKeyMapping(MoveLeftKeyMapping);

        FActionKeyMapping MoveRightKeyMapping("MoveRight", EKeys::D);
        Input->AddActionKeyMapping(MoveRightKeyMapping);

        FActionKeyMapping RotateUpKeyMapping("RotateUp", EKeys::Up);
        Input->AddActionKeyMapping(RotateUpKeyMapping);

        FActionKeyMapping RotateDownKeyMapping("RotateDown", EKeys::Down);
        Input->AddActionKeyMapping(RotateDownKeyMapping);

        FActionKeyMapping RotateLeftKeyMapping("RotateLeft", EKeys::Left);
        Input->AddActionKeyMapping(RotateLeftKeyMapping);

        FActionKeyMapping RotateRightKeyMapping("RotateRight", EKeys::Right);
        Input->AddActionKeyMapping(RotateRightKeyMapping);

        FActionKeyMapping JumpKeyMapping("Jump", EKeys::Space);
        Input->AddActionKeyMapping(JumpKeyMapping);
        
        FAxisKeyMapping MoveForwardAxisKeyMapping("MoveForwardAxis", EKeys::W, 1.0f);
        Input->AddAxisKeyMapping(MoveForwardAxisKeyMapping);
    }
}

void FSandboxPlayerController::Tick(float DeltaTime)
{
    GetPlayerInput()->EnableInput(InputComponent);

    // NOTE: The input is handled here via the input component
    Super::Tick(DeltaTime);

    const float RotationSpeed = 45.0f;
    const float Deadzone      = 0.01f;

    const FAxisState RightThumbX = GetPlayerInput()->GetAnalogState(EAnalogSourceName::RightThumbX);
    const FAxisState RightThumbY = GetPlayerInput()->GetAnalogState(EAnalogSourceName::RightThumbY);
    
    if (FMath::Abs(RightThumbX.Value) > Deadzone)
    {
        Camera->Rotate(0.0f, FMath::ToRadians(RightThumbX.Value * RotationSpeed * DeltaTime), 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::Right))
    {
        Camera->Rotate(0.0f, FMath::ToRadians(RotationSpeed * DeltaTime), 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::Left))
    {
        Camera->Rotate(0.0f, FMath::ToRadians(-RotationSpeed * DeltaTime), 0.0f);
    }

    if (FMath::Abs(RightThumbY.Value) > Deadzone)
    {
        Camera->Rotate(FMath::ToRadians(-RightThumbY.Value * RotationSpeed * DeltaTime), 0.0f, 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::Up))
    {
        Camera->Rotate(FMath::ToRadians(-RotationSpeed * DeltaTime), 0.0f, 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::Down))
    {
        Camera->Rotate(FMath::ToRadians(RotationSpeed * DeltaTime), 0.0f, 0.0f);
    }

    float Acceleration = 15.0f;
    if (GetPlayerInput()->IsKeyDown(EKeys::LeftShift) || GetPlayerInput()->IsKeyDown(EKeys::GamepadLeftTrigger))
    {
        Acceleration = Acceleration * 3;
    }

    const FAxisState LeftThumbX = GetPlayerInput()->GetAnalogState(EAnalogSourceName::LeftThumbX);
    const FAxisState LeftThumbY = GetPlayerInput()->GetAnalogState(EAnalogSourceName::LeftThumbY);

    FVector3 CameraAcceleration;
    if (FMath::Abs(LeftThumbY.Value) > Deadzone)
    {
        CameraAcceleration.z = Acceleration * LeftThumbY.Value;
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::W))
    {
        CameraAcceleration.z = Acceleration;
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::S))
    {
        CameraAcceleration.z = -Acceleration;
    }

    if (FMath::Abs(LeftThumbX.Value) > Deadzone)
    {
        CameraAcceleration.x = Acceleration * -LeftThumbX.Value;
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::A))
    {
        CameraAcceleration.x = Acceleration;
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::D))
    {
        CameraAcceleration.x = -Acceleration;
    }

    if (GetPlayerInput()->IsKeyDown(EKeys::Q))
    {
        CameraAcceleration.y = Acceleration;
    }
    else if (GetPlayerInput()->IsKeyDown(EKeys::E))
    {
        CameraAcceleration.y = -Acceleration;
    }

    const float Deacceleration = -5.0f;
    CameraSpeed = CameraSpeed + (CameraSpeed * Deacceleration) * DeltaTime;
    CameraSpeed = CameraSpeed + (CameraAcceleration * DeltaTime);

    const FVector3 Speed = CameraSpeed * DeltaTime;
    Camera->Move(Speed.x, Speed.y, Speed.z);
    Camera->UpdateMatrices();
}

void FSandboxPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("MoveForward", EActionState::Pressed, this, &FSandboxPlayerController::MoveForward);
    InputComponent->BindAction("MoveBackwards", EActionState::Pressed, this, &FSandboxPlayerController::MoveBackwards);
    InputComponent->BindAction("MoveRight", EActionState::Pressed, this, &FSandboxPlayerController::MoveRight);
    InputComponent->BindAction("MoveLeft", EActionState::Pressed, this, &FSandboxPlayerController::MoveLeft);

    InputComponent->BindAction("RotateUp", EActionState::Pressed, this, &FSandboxPlayerController::RotateUp);
    InputComponent->BindAction("RotateDown", EActionState::Pressed, this, &FSandboxPlayerController::RotateDown);
    InputComponent->BindAction("RotateRight", EActionState::Pressed, this, &FSandboxPlayerController::RotateRight);
    InputComponent->BindAction("RotateLeft", EActionState::Pressed, this, &FSandboxPlayerController::RotateLeft);

    InputComponent->BindAction("Jump", EActionState::Released, this, &FSandboxPlayerController::Jump);

    InputComponent->BindAxis("MoveForwardAxis", this, &FSandboxPlayerController::MoveForwardAxis);
}

void FSandboxPlayerController::MoveForwardAxis(float)
{
    // LOG_INFO("MoveForwardAxis %.4f", Value);
}

void FSandboxPlayerController::MoveForward()
{
    // LOG_INFO("MoveForward");
}

void FSandboxPlayerController::MoveBackwards()
{
    // LOG_INFO("MoveBackward");
}

void FSandboxPlayerController::MoveRight()
{
    // LOG_INFO("MoveRight");
}

void FSandboxPlayerController::MoveLeft()
{
    // LOG_INFO("MoveLeft");
}

void FSandboxPlayerController::RotateUp()
{
    // LOG_INFO("RotateUp");
}

void FSandboxPlayerController::RotateDown()
{
    // LOG_INFO("RotateDown");
}

void FSandboxPlayerController::RotateRight()
{
    // LOG_INFO("RotateRight");
}

void FSandboxPlayerController::RotateLeft()
{
    // LOG_INFO("RotateLeft");
}

void FSandboxPlayerController::Jump()
{
    // LOG_INFO("Jump");
}
