#pragma once
#include <Engine/Scene/Actors/Actor.h>
#include <Engine/Scene/Components/Component.h>

class FMovingBallComponent : public FComponent
{
    FOBJECT_BODY(FMovingBallComponent, FComponent);

public:

    FMovingBallComponent(FActor* InActorOwner, float InSpeed)
        : FComponent(InActorOwner, false, true)
        , Speed(InSpeed)
        , CurrentSpeed(InSpeed)
    { }

    virtual void Tick(FTimespan DeltaTime)
    {
        const float fDelta = float(DeltaTime.AsSeconds());

        FActor* Actor = GetActor();
        
        FActorTransform& ActorTransform = Actor->GetTransform();
        ActorTransform.SetTranslation(ActorTransform.GetTranslation() + FVector3(0.0f, CurrentSpeed * fDelta, 0.0f));

        if (ActorTransform.GetTranslation().y >= 60.0f)
        {
            CurrentSpeed = -Speed;
        }
        else if (ActorTransform.GetTranslation().y <= 0.0f)
        {
            CurrentSpeed = Speed;
        }
    }

private:
    float Speed;
    float CurrentSpeed;
};