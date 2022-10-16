#include "Component.h"

FComponent::FComponent(FActor* InActorOwner)
    : FCoreObject()
    , ActorOwner(InActorOwner)
    , bIsStartable(true)
    , bIsTickable(true)
{
    CHECK(InActorOwner != nullptr);
    CORE_OBJECT_INIT();
}

FComponent::FComponent(FActor* InActorOwner, bool bInIsStartable, bool bInIsTickable)
    : FCoreObject()
    , ActorOwner(InActorOwner)
    , bIsStartable(bInIsStartable)
    , bIsTickable(bInIsTickable)
{
    CHECK(InActorOwner != nullptr);
    CORE_OBJECT_INIT();
}
