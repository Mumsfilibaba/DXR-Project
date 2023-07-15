#pragma once
#include <Engine/Scene/Camera.h>
#include <Engine/Scene/Actors/PlayerController.h>

class SANDBOX_API FSandboxPlayerController : public FPlayerController
{
    FOBJECT_BODY(FSandboxPlayerController, FPlayerController);

public:
    FSandboxPlayerController(FScene* InScene);
    ~FSandboxPlayerController() = default;

    virtual void Tick(FTimespan DeltaTime) override;

    virtual void SetupInputComponent() override;

    void MoveForward();

    void MoveBackwards();

    void MoveRight();

    void MoveLeft();

    void RotateUp();

    void RotateDown();

    void RotateRight();

    void RotateLeft();

private:
    FCamera* Camera;
    FVector3 CameraSpeed;
};
