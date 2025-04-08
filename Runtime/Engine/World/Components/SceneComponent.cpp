#include "Engine/World/Components/SceneComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSceneComponent);

FSceneComponent::FSceneComponent(const FObjectInitializer& ObjectInitializer)
    : FActorComponent(ObjectInitializer)
{
    SetTickable(false);
    SetStartable(false);
}

FSceneComponent::~FSceneComponent()
{
}