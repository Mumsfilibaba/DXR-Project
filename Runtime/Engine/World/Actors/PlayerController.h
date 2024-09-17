#pragma once
#include "Actor.h"

class FPlayerInput;
class FInputComponent;

class ENGINE_API FPlayerController : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FPlayerController, FActor);

    FPlayerController(const FObjectInitializer& ObjectInitializer);
    ~FPlayerController();

    virtual void SetupInputComponent();
    virtual void Tick(float DeltaTime) override;

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
