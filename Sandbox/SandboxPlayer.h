#pragma once
#include "SandboxCore.h"

#include <Engine/Scene/Actors/PlayerController.h>

class SANDBOX_API FSandboxPlayerController
    : public FPlayerController
{
    FOBJECT_BODY(FSandboxPlayerController, FPlayerController);

public:
    FSandboxPlayerController(FScene* InScene);
    ~FSandboxPlayerController() = default;

    virtual void SetupInputComponent() override;

    void MoveForward();
    void MoveBackward();
    void MoveRight();
    void MoveLeft();

private:
    class FCamera* Camera;
};