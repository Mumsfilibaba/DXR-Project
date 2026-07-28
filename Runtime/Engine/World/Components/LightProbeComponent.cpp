#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/World.h"

FOBJECT_IMPLEMENT_CLASS(FLightProbeComponent);

FLightProbeComponent::FLightProbeComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , CubeMap(nullptr)
    , BoxOffset()
    , BoxExtent(1.0f, 1.0f, 1.0f)
    , bBoxProjection(false)
{
}

FLightProbeComponent::~FLightProbeComponent()
{
}

void FLightProbeComponent::SetBoxExtent(const Vector3& InBoxExtent)
{
    BoxExtent = InBoxExtent;
}

void FLightProbeComponent::SetBoxOffset(const Vector3& InBoxOffset)
{
    BoxOffset = InBoxOffset;
}

void FLightProbeComponent::SetBoxProjection(bool bInBoxProjection)
{
    bBoxProjection = bInBoxProjection;
}

void FLightProbeComponent::SetCubeMap(const FRHITextureRef& InCubeMap)
{
    if (CubeMap == InCubeMap)
    {
        return;
    }

    FWorld* World = GetActorOwner() ? GetActorOwner()->GetWorld() : nullptr;
    if (World && CubeMap)
    {
        World->RemoveSceneComponent(this);
    }

    CubeMap = InCubeMap;

    if (World && CubeMap)
    {
        World->AddSceneComponent(this);
    }
}

const Vector3& FLightProbeComponent::GetPosition() const
{
    CHECK(GetActorOwner() != nullptr);
    return GetActorOwner()->GetTransform().GetTranslation();
}
