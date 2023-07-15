#include "PlayerController.h"
#include "PlayerInput.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/Scene/Components/InputComponent.h"

FPlayerController::FPlayerController(FScene* InSceneOwner)
    : FActor(InSceneOwner)
    , InputComponent(nullptr)
    , PlayerInput(nullptr)
{
    FOBJECT_INIT();
    PlayerInput = new FPlayerInput();
}

FPlayerController::~FPlayerController()
{
    SAFE_DELETE(PlayerInput);
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
    // Call actor tick
    Super::Tick(DeltaTime);

    // Update the PlayerInput
    PlayerInput->Tick(DeltaTime);
}
