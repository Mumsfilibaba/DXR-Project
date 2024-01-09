#pragma once
#include <Engine/Scene/Actors/Actor.h>
#include <Engine/Scene/Components/Component.h>

class SANDBOX_API FMovingBallComponent : public FComponent
{
public:
    FOBJECT_DECLARE_CLASS(FMovingBallComponent, FComponent);

    FMovingBallComponent(const FObjectInitializer& ObjectInitializer);
    ~FMovingBallComponent() = default;

    virtual void Tick(FTimespan DeltaTime) override;

    float Speed;
    float CurrentSpeed;
};
