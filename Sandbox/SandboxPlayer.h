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
    void MoveBackward();
    void MoveRight();
    void MoveLeft();

private:
    FCamera* Camera;
    FVector3 CameraSpeed;
};