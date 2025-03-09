#include "Engine/World/Components/Component.h"

FOBJECT_IMPLEMENT_CLASS(FComponent);

FComponent::FComponent(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , bIsStartable(true)
    , bIsTickable(true)
{
}
