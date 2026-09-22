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
        Input->AddActionKeyMapping(FActionKeyMapping("Shoot", Keys::Space));
        Input->AddActionKeyMapping(FActionKeyMapping("Shoot", Keys::GamepadFaceDown));
        Input->AddActionKeyMapping(FActionKeyMapping("Boost", Keys::LeftShift));
        Input->AddActionKeyMapping(FActionKeyMapping("Boost", Keys::GamepadLeftThumb));

        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveForward", Keys::W, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveForward", Keys::S, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveRight", Keys::A, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveRight", Keys::D, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveUp", Keys::Q, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveUp", Keys::E, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveUp", Keys::GamepadLeftShoulder, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("MoveUp", Keys::GamepadRightShoulder, -1.0f));

        Input->AddAxisKeyMapping(FAxisKeyMapping("LookRight", Keys::Right, 1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("LookRight", Keys::Left, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("LookUp", Keys::Up, -1.0f));
        Input->AddAxisKeyMapping(FAxisKeyMapping("LookUp", Keys::Down, 1.0f));

        Input->AddAxisMapping(FAxisMapping("MoveForward", EAnalogSourceName::LeftThumbY, 1.0f));
        Input->AddAxisMapping(FAxisMapping("MoveRight", EAnalogSourceName::LeftThumbX, -1.0f));
        Input->AddAxisMapping(FAxisMapping("LookRight", EAnalogSourceName::RightThumbX, 1.0f));
        Input->AddAxisMapping(FAxisMapping("LookUp", EAnalogSourceName::RightThumbY, -1.0f));
        Input->AddAxisMapping(FAxisMapping("ShootTrigger", EAnalogSourceName::RightTrigger, 1.0f));
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

    const IntVector2 MouseDelta = Input->ConsumeMouseDelta();

    FFirstPersonCameraInput CameraInput;
    CameraInput.MoveAxis  = Vector3(
        Input->GetAxisValue("MoveRight"),
        Input->GetAxisValue("MoveUp"),
        Input->GetAxisValue("MoveForward"));

    CameraInput.StickLook = Vector2(
        Input->GetAxisValue("LookRight"),
        Input->GetAxisValue("LookUp"));
            
    CameraInput.LookDelta = Vector2(static_cast<float>(MouseDelta.X), static_cast<float>(MouseDelta.Y));
    CameraInput.bBoost    = bBoostHeld;

    CameraController.Tick(DeltaTime, CameraInput);
}

void FSandboxPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("Shoot", EActionState::Pressed, this, &FSandboxPlayerController::Shoot);
    InputComponent->BindAction("Boost", EActionState::Pressed, this, &FSandboxPlayerController::BoostPressed);
    InputComponent->BindAction("Boost", EActionState::Released, this, &FSandboxPlayerController::BoostReleased);
    InputComponent->BindAxis("ShootTrigger", this, &FSandboxPlayerController::OnShootTriggerAxis);
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
