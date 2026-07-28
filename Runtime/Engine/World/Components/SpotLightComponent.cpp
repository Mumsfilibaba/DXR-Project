#include "Engine/World/Components/SpotLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSpotLightComponent);

FSpotLightComponent::FSpotLightComponent(const FObjectInitializer& ObjectInitializer)
    : FLightComponent(ObjectInitializer)
    , ConeAngle(45.0f)
{
}

FSpotLightComponent::~FSpotLightComponent()
{
}

void FSpotLightComponent::SetConeAngle(float InConeAngle)
{
    ConeAngle = InConeAngle;
}
