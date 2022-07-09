#pragma once
#include <Engine/Scene/Actor.h>
#include <Engine/Scene/Components/Component.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMovingBallComponent

class FMovingBallComponent : public CComponent
{
    CORE_OBJECT(FMovingBallComponent, CComponent);

public:

    FMovingBallComponent(FActor* InActorOwner, float InSpeed)
        : CComponent(InActorOwner, false, true)
        , Speed(InSpeed)
        , CurrentSpeed(InSpeed)
    { }

    virtual void Tick(FTimestamp DeltaTime)
    {
        const float fDelta = float(DeltaTime.AsSeconds());

        FActor* Actor = GetActor();
        
        CActorTransform& ActorTransform = Actor->GetTransform();
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