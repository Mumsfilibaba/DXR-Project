#include "Engine/World/Components/KinematicMovementComponent.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/World.h"

FOBJECT_IMPLEMENT_CLASS(FKinematicMovementComponent);

FKinematicMovementComponent::FKinematicMovementComponent(const FObjectInitializer& ObjectInitializer)
    : FActorComponent(ObjectInitializer)
    , Velocity()
    , Gravity(-20.0f)
    , DespawnBelowY(-50.0f)
{
}

void FKinematicMovementComponent::SetInitialVelocity(const Vector3& InVelocity)
{
    Velocity = InVelocity;
}

void FKinematicMovementComponent::SetGravity(float InGravity)
{
    Gravity = InGravity;
}

void FKinematicMovementComponent::SetDespawnBelowY(float InDespawnBelowY)
{
    DespawnBelowY = InDespawnBelowY;
}

void FKinematicMovementComponent::Tick(float DeltaTime)
{
    FActor* Actor = GetActorOwner();
    if (!Actor)
    {
        return;
    }

    Velocity.Y += Gravity * DeltaTime;

    FActorTransform& ActorTransform = Actor->GetTransform();
    ActorTransform.SetTranslation(ActorTransform.GetTranslation() + (Velocity * DeltaTime));

    if (ActorTransform.GetTranslation().Y < DespawnBelowY)
    {
        if (FWorld* World = Actor->GetWorld())
        {
            World->DestroyActorDeferred(Actor);
        }
    }
}
