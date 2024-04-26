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

    virtual void Start() { }
    virtual void Tick(FTimespan DeltaTime) { }

    FActor* GetActorOwner() const
    {
        return ActorOwner;
    }

    void SetActorOwner(FActor* InActorOwner) 
    {
        ActorOwner = InActorOwner;
    }

    bool IsStartable() const
    {
        return bIsStartable;
    }

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
