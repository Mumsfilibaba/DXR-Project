#include "Engine/World/Components/SkyLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSkyLightComponent);

FSkyLightComponent::FSkyLightComponent(const FObjectInitializer& ObjectInitializer)
    : FLightComponent(ObjectInitializer)
    , Source(ESkyLightSource::FollowSkybox)
    , Environment()
    , Rotation(0.0f)
{
}

FSkyLightComponent::~FSkyLightComponent()
{
}

void FSkyLightComponent::SetSource(ESkyLightSource InSource)
{
    Source = InSource;
}

void FSkyLightComponent::SetEnvironment(const TSharedPtr<FEnvironment>& InEnvironment)
{
    Environment = InEnvironment;
}

void FSkyLightComponent::SetRotation(float InRotation)
{
    Rotation = InRotation;
}
