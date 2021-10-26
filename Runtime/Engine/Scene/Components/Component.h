#pragma once
#include "Core/Time/Timestamp.h"

#include "Engine/CoreObject/CoreObject.h"

class CActor;

// CComponent BaseClass
class ENGINE_API CComponent : public CCoreObject
{
    CORE_OBJECT( CComponent, CCoreObject );

public:

    CComponent( CActor* InActorOwner );
    virtual ~CComponent() = default;

    /* Start component, called in the beginning of the run, perform initialization here */
    virtual void Start();

    /* Tick component, should be called once every frame */
    virtual void Tick( CTimestamp DeltaTime );

    FORCEINLINE CActor* GetActor() const
    {
        return ActorOwner;
    }

    FORCEINLINE bool IsStartable() const
    {
        return Startable;
    }

    FORCEINLINE bool IsTickable() const
    {
        return Tickable;
    }

protected:

    /* The actor that this component belongs to */
    CActor* ActorOwner = nullptr;

    /* Flags for this component that decides if it should start or not */
    bool Startable : 1;

    /* Flags for this component that decides if it should tick or not */
    bool Tickable : 1;
};