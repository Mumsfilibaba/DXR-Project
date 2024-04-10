#include "SceneComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSceneComponent);

FSceneComponent::FSceneComponent(const FObjectInitializer& ObjectInitializer)
    : FComponent(ObjectInitializer)
{
    SetTickable(false);
    SetStartable(false);
}