#include "SandboxPlayer.h"
#include <Core/Misc/OutputDeviceLogger.h>
#include <Engine/Engine.h>
#include <Engine/World/World.h>
#include <Engine/World/Actors/CameraActor.h>
#include <Engine/World/Actors/PlayerInput.h>
#include <Engine/World/Components/CameraComponent.h>
#include <Engine/World/Components/InputComponent.h>

FOBJECT_IMPLEMENT_CLASS(FSandboxPlayerController);

FSandboxPlayerController::FSandboxPlayerController(const FObjectInitializer& Initializer)
    : FPlayerController(Initializer)
    , CameraActor(nullptr)
    , CameraController()
{
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

    if (!CameraActor)
    {
        return;
    }

    FPlayerInput* Input = GetPlayerInput();
    CameraController.SetCamera(CameraActor->GetCameraComponent());

    // Reset Camera
    if (Input->IsKeyDown(Keys::R))
    {
        FCameraComponent* Camera = CameraActor->GetCameraComponent();
        Camera->SetPosition(0.0f, 10.0f, -2.0f);
        Camera->SetRotation(0.0f,  0.0f,  0.0f);

        CameraController.StopMovement();
        CameraController.SyncFromCamera();
    }

    const FAxisState RightThumbX = Input->GetAnalogState(EAnalogSourceName::RightThumbX);
    const FAxisState RightThumbY = Input->GetAnalogState(EAnalogSourceName::RightThumbY);

    const IntVector2 MouseDelta = Input->ConsumeMouseDelta();

    FFirstPersonCameraInput CameraInput;
    CameraInput.MoveAxis  = GatherMoveAxis(Input);
    CameraInput.LookDelta = Vector2(static_cast<float>(MouseDelta.X), static_cast<float>(MouseDelta.Y));
    CameraInput.StickLook = Vector2(RightThumbX.Value, -RightThumbY.Value);
    CameraInput.bBoost    = Input->IsKeyDown(Keys::LeftShift) || Input->IsKeyDown(Keys::GamepadLeftThumb);

    // Arrow keys still turn the camera, at the same rate the look stick does
    if (Input->IsKeyDown(Keys::Right))
    {
        CameraInput.StickLook.X += 1.0f;
    }
    else if (Input->IsKeyDown(Keys::Left))
    {
        CameraInput.StickLook.X -= 1.0f;
    }

    if (Input->IsKeyDown(Keys::Up))
    {
        CameraInput.StickLook.Y -= 1.0f;
    }
    else if (Input->IsKeyDown(Keys::Down))
    {
        CameraInput.StickLook.Y += 1.0f;
    }

    CameraController.Tick(DeltaTime, CameraInput);
}

Vector3 FSandboxPlayerController::GatherMoveAxis(const FPlayerInput* Input) const
{
    constexpr float Deadzone = 0.01f;

    const FAxisState LeftThumbX = Input->GetAnalogState(EAnalogSourceName::LeftThumbX);
    const FAxisState LeftThumbY = Input->GetAnalogState(EAnalogSourceName::LeftThumbY);

    Vector3 MoveAxis;
    if (Math::Abs(LeftThumbY.Value) > Deadzone)
    {
        MoveAxis.Z = LeftThumbY.Value;
    }
    else if (Input->IsKeyDown(Keys::W))
    {
        MoveAxis.Z = 1.0f;
    }
    else if (Input->IsKeyDown(Keys::S))
    {
        MoveAxis.Z = -1.0f;
    }

    if (Math::Abs(LeftThumbX.Value) > Deadzone)
    {
        MoveAxis.X = -LeftThumbX.Value;
    }
    else if (Input->IsKeyDown(Keys::A))
    {
        MoveAxis.X = 1.0f;
    }
    else if (Input->IsKeyDown(Keys::D))
    {
        MoveAxis.X = -1.0f;
    }

    if (Input->IsKeyDown(Keys::Q))
    {
        MoveAxis.Y = 1.0f;
    }
    else if (Input->IsKeyDown(Keys::E))
    {
        MoveAxis.Y = -1.0f;
    }

    return MoveAxis;
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
