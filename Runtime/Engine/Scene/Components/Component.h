#pragma once
#include "Core/Time/Timespan.h"
#include "Engine/Core/Object.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FActor;

class ENGINE_API FComponent : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FComponent, FObject);

    FComponent(const FObjectInitializer& ObjectInitializer);
    virtual ~FComponent() = default;

    /**
     * @brief - Start component, called in the beginning of the run, perform initialization here
     */
    virtual void Start() { }

    /**
     * @brief           - Tick component, should be called once every frame 
     * @param DeltaTime - Time since the last call to tick
     */
    virtual void Tick(FTimespan DeltaTime) { }

    /**
     * @brief  - Retrieve the actor that the component belongs to
     * @return - Returns a pointer to the actor that the component belongs to
     */
    FActor* GetActorOwner() const
    {
        return ActorOwner;
    }

    void SetActorOwner(FActor* InActorOwner) 
    {
        ActorOwner = InActorOwner;
    }

    /**
     * @brief  - Check if Start should be called on the component
     * @return - Returns true if the component's Start-method should be called 
     */
    bool IsStartable() const
    {
        return bIsStartable;
    }

    /**
     * @brief  - Check if Tick should be called on the component
     * @return - Returns true if the component's Tick-method should be called
     */
    bool IsTickable() const
    {
        return bIsTickable;
    }

    void SetStartable(bool bInIsStartable)
    {
        bIsStartable = bInIsStartable;
    }

    void SetTickable(bool bInIsTickable)
    {
        bIsTickable = bInIsTickable;
    }

protected:
    bool bIsStartable : 1;
    bool bIsTickable  : 1;

private:
    FActor* ActorOwner = nullptr;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
