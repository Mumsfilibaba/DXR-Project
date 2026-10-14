#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/World.h"

FOBJECT_IMPLEMENT_CLASS(FLightProbeComponent);

FLightProbeComponent::FLightProbeComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Environment()
    , BoxOffset()
    , BoxExtent(1.0f, 1.0f, 1.0f)
    , Intensity(1.0f)
    , Rotation(0.0f)
    , bBoxProjection(false)
{
}

FLightProbeComponent::~FLightProbeComponent()
{
}

void FLightProbeComponent::SetBoxExtent(const FVector3& InBoxExtent)
{
    BoxExtent = InBoxExtent;
}

void FLightProbeComponent::SetBoxOffset(const FVector3& InBoxOffset)
{
    BoxOffset = InBoxOffset;
}

void FLightProbeComponent::SetBoxProjection(bool bInBoxProjection)
{
    bBoxProjection = bInBoxProjection;
}

void FLightProbeComponent::SetEnvironment(const TSharedPtr<FEnvironment>& InEnvironment)
{
    if (Environment == InEnvironment)
    {
        return;
    }

    // A probe is in the scene while it has an environment; swapping one environment for another reaches it with the next frame's update
    FWorld* World = GetActorOwner() ? GetActorOwner()->GetWorld() : nullptr;
    if (World && Environment && !InEnvironment)
    {
        World->RemoveSceneComponent(this);
    }

    const bool bAdd = !Environment && InEnvironment;
    Environment = InEnvironment;

    if (World && bAdd)
    {
        World->AddSceneComponent(this);
    }
}

void FLightProbeComponent::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}

void FLightProbeComponent::SetRotation(float InRotation)
{
    Rotation = InRotation;
}

const FVector3& FLightProbeComponent::GetPosition() const
{
    CHECK(GetActorOwner() != nullptr);
    return GetActorOwner()->GetWorldTransform().GetTranslation();
}
