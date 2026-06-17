#include "SandboxPlayer.h"
#include <Core/Misc/OutputDeviceLogger.h>
#include <Engine/Engine.h>
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
        FActionKeyMapping MoveForwardKeyMapping("MoveForward", Keys::W);
        Input->AddActionKeyMapping(MoveForwardKeyMapping);

        FActionKeyMapping MoveBackwardsKeyMapping("MoveBackwards", Keys::S);
        Input->AddActionKeyMapping(MoveBackwardsKeyMapping);

        FActionKeyMapping MoveLeftKeyMapping("MoveLeft", Keys::A);
        Input->AddActionKeyMapping(MoveLeftKeyMapping);

        FActionKeyMapping MoveRightKeyMapping("MoveRight", Keys::D);
        Input->AddActionKeyMapping(MoveRightKeyMapping);

        FActionKeyMapping RotateUpKeyMapping("RotateUp", Keys::Up);
        Input->AddActionKeyMapping(RotateUpKeyMapping);

        FActionKeyMapping RotateDownKeyMapping("RotateDown", Keys::Down);
        Input->AddActionKeyMapping(RotateDownKeyMapping);

        FActionKeyMapping RotateLeftKeyMapping("RotateLeft", Keys::Left);
        Input->AddActionKeyMapping(RotateLeftKeyMapping);

        FActionKeyMapping RotateRightKeyMapping("RotateRight", Keys::Right);
        Input->AddActionKeyMapping(RotateRightKeyMapping);

        FActionKeyMapping JumpKeyMapping("Jump", Keys::Space);
        Input->AddActionKeyMapping(JumpKeyMapping);
        
        FAxisKeyMapping MoveForwardAxisKeyMapping("MoveForwardAxis", Keys::W, 1.0f);
        Input->AddAxisKeyMapping(MoveForwardAxisKeyMapping);
    }
}

FSandboxPlayerController::~FSandboxPlayerController()
{
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
    
    // Reset Camera
    if (GetPlayerInput()->IsKeyDown(Keys::R))
    {
        Camera->SetPosition(0.0f, 10.0f, -2.0f);
        Camera->SetRotation(0.0f,  0.0f,  0.0f);
    }
    
    // Camera Rotation
    if (Math::Abs(RightThumbX.Value) > Deadzone)
    {
        Camera->Rotate(0.0f, Math::DegreesToRadians(RightThumbX.Value * RotationSpeed * DeltaTime), 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::Right))
    {
        Camera->Rotate(0.0f, Math::DegreesToRadians(RotationSpeed * DeltaTime), 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::Left))
    {
        Camera->Rotate(0.0f, Math::DegreesToRadians(-RotationSpeed * DeltaTime), 0.0f);
    }

    if (Math::Abs(RightThumbY.Value) > Deadzone)
    {
        Camera->Rotate(Math::DegreesToRadians(-RightThumbY.Value * RotationSpeed * DeltaTime), 0.0f, 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::Up))
    {
        Camera->Rotate(Math::DegreesToRadians(-RotationSpeed * DeltaTime), 0.0f, 0.0f);
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::Down))
    {
        Camera->Rotate(Math::DegreesToRadians(RotationSpeed * DeltaTime), 0.0f, 0.0f);
    }

    // Camera Movement
    float Acceleration = 15.0f;
    if (GetPlayerInput()->IsKeyDown(Keys::LeftShift) || GetPlayerInput()->IsKeyDown(Keys::GamepadLeftThumb))
    {
        Acceleration = Acceleration * 3;
    }

    const FAxisState LeftThumbX = GetPlayerInput()->GetAnalogState(EAnalogSourceName::LeftThumbX);
    const FAxisState LeftThumbY = GetPlayerInput()->GetAnalogState(EAnalogSourceName::LeftThumbY);

    Vector3 CameraAcceleration;
    if (Math::Abs(LeftThumbY.Value) > Deadzone)
    {
        CameraAcceleration.Z = Acceleration * LeftThumbY.Value;
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::W))
    {
        CameraAcceleration.Z = Acceleration;
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::S))
    {
        CameraAcceleration.Z = -Acceleration;
    }

    if (Math::Abs(LeftThumbX.Value) > Deadzone)
    {
        CameraAcceleration.X = Acceleration * -LeftThumbX.Value;
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::A))
    {
        CameraAcceleration.X = Acceleration;
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::D))
    {
        CameraAcceleration.X = -Acceleration;
    }

    if (GetPlayerInput()->IsKeyDown(Keys::Q))
    {
        CameraAcceleration.Y = Acceleration;
    }
    else if (GetPlayerInput()->IsKeyDown(Keys::E))
    {
        CameraAcceleration.Y = -Acceleration;
    }

    const float DampingRate   = 5.0f;
    const float DampingFactor = Math::Exp(-DampingRate * DeltaTime);
    CameraSpeed = CameraSpeed * DampingFactor;
    CameraSpeed = CameraSpeed + (CameraAcceleration * DeltaTime);

    const Vector3 Speed = CameraSpeed * DeltaTime;
    Camera->Move(Speed.X, Speed.Y, Speed.Z);

    // When the camera has rotated and moved we can update the view matrix
    Camera->UpdateViewMatrix();
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
