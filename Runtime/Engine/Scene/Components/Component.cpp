#include "Component.h"

FComponent::FComponent(FActor* InActorOwner)
    : FObject()
    , ActorOwner(InActorOwner)
    , bIsStartable(true)
    , bIsTickable(true)
{
    CHECK(InActorOwner != nullptr);
    FOBJECT_INIT();
}

FComponent::FComponent(FActor* InActorOwner, bool bInIsStartable, bool bInIsTickable)
    : FObject()
    , ActorOwner(InActorOwner)
    , bIsStartable(bInIsStartable)
    , bIsTickable(bInIsTickable)
{
    CHECK(InActorOwner != nullptr);
    FOBJECT_INIT();
}
