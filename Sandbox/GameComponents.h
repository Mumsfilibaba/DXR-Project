#pragma once
#include <Engine/World/Actors/Actor.h>
#include <Engine/World/Components/ActorComponent.h>

class SANDBOX_API FMovingBallComponent : public FActorComponent
{
public:
    FOBJECT_DECLARE_CLASS(FMovingBallComponent, FActorComponent);

    FMovingBallComponent(const FObjectInitializer& ObjectInitializer);
    ~FMovingBallComponent() = default;

    virtual void Tick(float DeltaTime) override;

    float Speed;
    float CurrentSpeed;
};
