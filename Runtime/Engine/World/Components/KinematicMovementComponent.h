#pragma once
#include "Core/Math/Vector3.h"
#include "Engine/World/Components/ActorComponent.h"

class ENGINE_API FKinematicMovementComponent : public FActorComponent
{
public:
    FOBJECT_DECLARE_CLASS(FKinematicMovementComponent, FActorComponent);

    FKinematicMovementComponent(const FObjectInitializer& ObjectInitializer);
    ~FKinematicMovementComponent() = default;

    // FActorComponent Interface
    virtual void Tick(float DeltaTime) override;

    void SetInitialVelocity(const FVector3& InVelocity);
    void SetGravity(float InGravity);
    void SetDespawnBelowY(float InDespawnBelowY);

    const FVector3& GetVelocity() const
    {
        return Velocity;
    }

    float GetGravity() const
    {
        return Gravity;
    }

    float GetDespawnBelowY() const
    {
        return DespawnBelowY;
    }

private:
    FVector3 Velocity;
    float    Gravity;
    float    DespawnBelowY;
};
