#pragma once
#include <Engine/World/Actors/PlayerController.h>
#include <Engine/World/FirstPersonCameraController.h>

class FCameraActor;

class SANDBOX_API FSandboxPlayerController : public FPlayerController
{
public:
    FOBJECT_DECLARE_CLASS(FSandboxPlayerController, FPlayerController);

    FSandboxPlayerController(const FObjectInitializer& Initializer);
    ~FSandboxPlayerController();

    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    void Shoot();
    void BoostPressed();
    void BoostReleased();
    void OnShootTriggerAxis(float Value);

    void SetCameraActor(FCameraActor* InCameraActor)
    {
        CameraActor = InCameraActor;
    }

    FCameraActor* GetCameraActor() const
    {
        return CameraActor;
    }

private:
    FCameraActor*                CameraActor;
    FFirstPersonCameraController CameraController;
    bool                         bBoostHeld;
    bool                         bWasShootTriggerDown;
};
