#include "Component.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

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

CComponent::CComponent(CActor* InActorOwner, bool bInIsStartable, bool bInIsTickable)
    : CCoreObject()
    , ActorOwner(InActorOwner)
    , bIsStartable(bInIsStartable)
    , bIsTickable(bInIsTickable)
{
    Check(InActorOwner != nullptr);

    CORE_OBJECT_INIT();
}

void CComponent::Start()
{
}

void CComponent::Tick(CTimestamp DeltaTime)
{
}

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif