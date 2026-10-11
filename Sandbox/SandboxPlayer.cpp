#include "SandboxPlayer.h"
#include "SandboxProjectile.h"
#include <Core/Misc/OutputDeviceManager.h>
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
    , bBoostHeld(false)
    , bWasShootTriggerDown(false)
{
    if (FPlayerInput* Input = GetPlayerInput())
    {
        Input->AddActionKeyMapping(FActionKeyMapping(TEXT("Shoot"), Keys::Space));
        Input->AddActionKeyMapping(FActionKeyMapping(TEXT("Shoot"), Keys::GamepadFaceDown));
        Input->AddActionKeyMapping(FActionKeyMapping(TEXT("Boost"), Keys::LeftShift));
        Input->AddActionKeyMapping(FActionKeyMapping(TEXT("Boost"), Keys::GamepadLeftThumb));

        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveForward"), Keys::W, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveForward"), Keys::S, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveRight"), Keys::A, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveRight"), Keys::D, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveUp"), Keys::Q, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveUp"), Keys::E, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveUp"), Keys::GamepadLeftShoulder, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("MoveUp"), Keys::GamepadRightShoulder, -1.0f));

        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("LookRight"), Keys::Right, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("LookRight"), Keys::Left, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("LookUp"), Keys::Up, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping(TEXT("LookUp"), Keys::Down, 1.0f));

        Input->AddAxisMapping(FAxisMapping(TEXT("MoveForward"), EAnalogSourceName::LeftThumbY, 1.0f));
        Input->AddAxisMapping(FAxisMapping(TEXT("MoveRight"), EAnalogSourceName::LeftThumbX, -1.0f));
        Input->AddAxisMapping(FAxisMapping(TEXT("LookRight"), EAnalogSourceName::RightThumbX, 1.0f));
        Input->AddAxisMapping(FAxisMapping(TEXT("LookUp"), EAnalogSourceName::RightThumbY, -1.0f));
        Input->AddAxisMapping(FAxisMapping(TEXT("ShootTrigger"), EAnalogSourceName::RightTrigger, 1.0f));
    }
}

FSandboxPlayerController::~FSandboxPlayerController()
{
}

void FSandboxPlayerController::Tick(float DeltaTime)
{
    GetPlayerInput()->EnableInput(InputComponent);

    Super::Tick(DeltaTime);

    if (!CameraActor)
    {
        return;
    }

    FPlayerInput* Input = GetPlayerInput();
    CameraController.SetCamera(CameraActor->GetCameraComponent());

    if (Input->IsKeyDown(Keys::R))
    {
        FCameraComponent* Camera = CameraActor->GetCameraComponent();
        Camera->SetPosition(0.0f, 10.0f, -2.0f);
        Camera->SetRotation(0.0f, 0.0f, 0.0f);

        CameraController.StopMovement();
        CameraController.SyncFromCamera();
    }

    const FIntVector2 MouseDelta = Input->ConsumeMouseDelta();

    FFirstPersonCameraInput CameraInput;
    CameraInput.MoveAxis  = FVector3(
        Input->GetAxisValue(TEXT("MoveRight")),
        Input->GetAxisValue(TEXT("MoveUp")),
        Input->GetAxisValue(TEXT("MoveForward")));

    CameraInput.StickLook = FVector2(
        Input->GetAxisValue(TEXT("LookRight")),
        Input->GetAxisValue(TEXT("LookUp")));
            
    CameraInput.LookDelta = FVector2(static_cast<float>(MouseDelta.X), static_cast<float>(MouseDelta.Y));
    CameraInput.bBoost    = bBoostHeld;

    CameraController.Tick(DeltaTime, CameraInput);
}

void FSandboxPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction(TEXT("Shoot"), EActionState::Pressed, this, &FSandboxPlayerController::Shoot);
    InputComponent->BindAction(TEXT("Boost"), EActionState::Pressed, this, &FSandboxPlayerController::BoostPressed);
    InputComponent->BindAction(TEXT("Boost"), EActionState::Released, this, &FSandboxPlayerController::BoostReleased);
    InputComponent->BindAxis(TEXT("ShootTrigger"), this, &FSandboxPlayerController::OnShootTriggerAxis);
}

void FSandboxPlayerController::Shoot()
{
    if (!CameraActor)
    {
        return;
    }

    FCameraComponent* Camera = CameraActor->GetCameraComponent();
    SpawnProjectileSphere(GetWorld(), Camera);
}

void FSandboxPlayerController::BoostPressed()
{
    bBoostHeld = true;
}

void FSandboxPlayerController::BoostReleased()
{
    bBoostHeld = false;
}

void FSandboxPlayerController::OnShootTriggerAxis(float Value)
{
    const bool bIsDown = Value > 0.5f;
    if (bIsDown && !bWasShootTriggerDown)
    {
        Shoot();
    }

    bWasShootTriggerDown = bIsDown;
}
