#pragma once
#include <Engine/World/Actors/PlayerController.h>

class FCameraActor;

class SANDBOX_API FSandboxPlayerController : public FPlayerController
{
public:
    FOBJECT_DECLARE_CLASS(FSandboxPlayerController, FPlayerController);

    FSandboxPlayerController(const FObjectInitializer& Initializer);
    ~FSandboxPlayerController();

    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    void MoveForward();
    void MoveForwardAxis(float Value);
    void MoveBackwards();
    void MoveRight();
    void MoveLeft();

    void RotateUp();
    void RotateDown();
    void RotateRight();
    void RotateLeft();

    void Jump();

    void SetCameraActor(FCameraActor* InCameraActor)
    {
        CameraActor = InCameraActor;
    }

    FCameraActor* GetCameraActor() const
    {
        return CameraActor;
    }

private:
    FCameraActor* CameraActor;
    Vector3       CameraSpeed;
};
