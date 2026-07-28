#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/SkyLightComponent.h"
#include "Engine/World/World.h"

FOBJECT_IMPLEMENT_CLASS(FSkyLightComponent);

FSkyLightComponent::FSkyLightComponent(const FObjectInitializer& ObjectInitializer)
    : FLightComponent(ObjectInitializer)
    , CubeMap(nullptr)
{
}

FSkyLightComponent::~FSkyLightComponent()
{
}

void FSkyLightComponent::SetCubeMap(const FRHITextureRef& InCubeMap)
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
