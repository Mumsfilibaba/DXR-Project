#include "Component.h"

CComponent::CComponent( CActor* InActorOwner )
    : CCoreObject()
    , ActorOwner( InActorOwner )
    , Startable( true )
    , Tickable( true )
{
    Assert( InActorOwner != nullptr );
    CORE_OBJECT_INIT();
}

void CComponent::Start()
{
}

void CComponent::Tick( CTimestamp DeltaTime )
{
    UNREFERENCED_VARIABLE( DeltaTime );
}