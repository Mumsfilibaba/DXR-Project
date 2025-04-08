#include "Engine/World/Components/ActorComponent.h"

FOBJECT_IMPLEMENT_CLASS(FActorComponent);

FActorComponent::FActorComponent(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , bIsStartable(true)
    , bIsTickable(true)
{
}
