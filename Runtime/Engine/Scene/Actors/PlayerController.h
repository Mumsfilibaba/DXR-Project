#pragma once
#include "Actor.h"

class FPlayerInput;
class FInputComponent;

class ENGINE_API FPlayerController : public FActor
{
    FOBJECT_BODY(FPlayerController, FActor);

public:
    FPlayerController(FScene* InSceneOwner);
    ~FPlayerController();

    virtual void SetupInputComponent();

    virtual void Tick(FTimespan DeltaTime) override;

    FInputComponent* GetInputComponent() const
    {
        CHECK(InputComponent != nullptr);
        return InputComponent;
    }

    FPlayerInput* GetPlayerInput() const
    {
        CHECK(PlayerInput != nullptr);
        return PlayerInput;
    }

protected:
    FInputComponent* InputComponent;

private:
    FPlayerInput* PlayerInput;
};
