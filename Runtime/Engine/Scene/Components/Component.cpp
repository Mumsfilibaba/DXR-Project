#include "Component.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FComponent

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

void FComponent::Start()
{
}

void FComponent::Tick(FTimespan DeltaTime)
{
}

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif