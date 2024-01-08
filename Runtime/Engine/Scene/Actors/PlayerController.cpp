#include "PlayerController.h"
#include "PlayerInput.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/Scene/Components/InputComponent.h"

FOBJECT_IMPLEMENT_CLASS(FPlayerController);

FPlayerController::FPlayerController(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , InputComponent(nullptr)
    , PlayerInput(nullptr)
{
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
        InputComponent = NewObject<FInputComponent>();
        if (InputComponent)
        {
            AddComponent(InputComponent);
        }
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
