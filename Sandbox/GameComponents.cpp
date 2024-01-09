#include "GameComponents.h"

FOBJECT_IMPLEMENT_CLASS(FMovingBallComponent)

FMovingBallComponent::FMovingBallComponent(const FObjectInitializer& ObjectInitializer)
    : FComponent(ObjectInitializer)
    , Speed(0.0f)
    , CurrentSpeed(0.0f)
{
}

void FMovingBallComponent::Tick(FTimespan DeltaTime)
{
    const float fDelta = float(DeltaTime.AsSeconds());

    FActor* Actor = GetActorOwner();
    
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