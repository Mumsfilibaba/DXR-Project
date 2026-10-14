#include "SkyboxComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSkyboxComponent);

FSkyboxComponent::FSkyboxComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Environment()
    , Intensity(1.0f)
    , Rotation(0.0f)
{
}

void FSkyboxComponent::SetEnvironment(const TSharedPtr<FEnvironment>& InEnvironment)
{
    Environment = InEnvironment;
}

void FSkyboxComponent::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}

void FSkyboxComponent::SetRotation(float InRotation)
{
    Rotation = InRotation;
}
