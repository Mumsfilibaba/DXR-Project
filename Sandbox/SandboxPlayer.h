#pragma once
#include <Engine/Scene/Camera.h>
#include <Engine/Scene/Actors/PlayerController.h>

class SANDBOX_API FSandboxPlayerController : public FPlayerController
{
public:
    FOBJECT_DECLARE_CLASS(FSandboxPlayerController, FPlayerController, SANDBOX_API);

    FSandboxPlayerController(const FObjectInitializer& Initializer);
    ~FSandboxPlayerController() = default;

    virtual void Tick(FTimespan DeltaTime) override;

    virtual void SetupInputComponent() override;

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

    FCamera* GetCamera() const
    {
        return Camera;
    }
    
private:
    FCamera* Camera;
    FVector3 CameraSpeed;
};
