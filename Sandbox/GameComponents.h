#pragma once
#include <Engine/Scene/Actor.h>
#include <Engine/Scene/Components/Component.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMovingBallComponent

class CMovingBallComponent : public CComponent
{
    CORE_OBJECT(CMovingBallComponent, CComponent);

public:

    CMovingBallComponent(CActor* InActorOwner, float InSpeed)
        : CComponent(InActorOwner, false, true)
        , Speed(InSpeed)
        , CurrentSpeed(InSpeed)
    { }

    virtual void Tick(CTimestamp DeltaTime)
    {
        const float fDelta = float(DeltaTime.AsSeconds());

        CActor* Actor = GetActor();
        
        CActorTransform& ActorTransform = Actor->GetTransform();
        ActorTransform.SetTranslation(ActorTransform.GetTranslation() + CVector3(0.0f, CurrentSpeed * fDelta, 0.0f));

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