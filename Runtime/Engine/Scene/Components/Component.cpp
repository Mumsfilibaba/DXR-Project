#include "Component.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CComponent

CComponent::CComponent(CActor* InActorOwner)
    : CCoreObject()
    , ActorOwner(InActorOwner)
    , bIsStartable(true)
    , bIsTickable(true)
{
    Check(InActorOwner != nullptr);
    CORE_OBJECT_INIT();
}

void CComponent::Start()
{
}

void CComponent::Tick(CTimestamp DeltaTime)
{
    UNREFERENCED_VARIABLE(DeltaTime);
}