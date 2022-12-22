#include "PlayerController.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FPlayerController::FPlayerController(FScene* InSceneOwner)
    : FActor(InSceneOwner)
    , InputComponent(nullptr)
    , PlayerInput(nullptr)
{
    FOBJECT_INIT();
}

void FPlayerController::SetupInputComponent()
{
    if (!InputComponent)
    {
        InputComponent = new FInputComponent(this);
        AddComponent(InputComponent);
    }

    CHECK(InputComponent != nullptr);
}

void FPlayerController::Tick(FTimespan DeltaTime)
{
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
