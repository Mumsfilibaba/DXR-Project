#pragma once
#include "Core/Time/Timestamp.h"

#include "Engine/CoreObject/CoreObject.h"

class CActor;

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CComponent

class ENGINE_API CComponent : public CCoreObject
{
    CORE_OBJECT(CComponent, CCoreObject);

public:

    CComponent(CActor* InActorOwner);
    virtual ~CComponent() = default;

    /** Start component, called in the beginning of the run, perform initialization here */
    virtual void Start();

    /**
     * Tick component, should be called once every frame 
     * 
     * @param DeltaTime: Time since the last call to tick
     */
    virtual void Tick(CTimestamp DeltaTime);

    /**
     * Retrieve the actor that the component belongs to
     * 
     * @return: Returns a pointer to the actor that the component belongs to
     */
    FORCEINLINE CActor* GetActor() const
    {
        return ActorOwner;
    }

    /**
     * Check if Start should be called on the component
     * 
     * @return: Returns true if the component's Start-method should be called 
     */
    FORCEINLINE bool IsStartable() const
    {
        return bIsStartable;
    }

    /**
     * Check if Tick should be called on the component
     *
     * @return: Returns true if the component's Tick-method should be called
     */
    FORCEINLINE bool IsTickable() const
    {
        return bIsTickable;
    }

protected:
    CActor* ActorOwner = nullptr;

    bool bIsStartable : 1;
    bool bIsTickable : 1;
};