#include "SpotLight.h"

FOBJECT_IMPLEMENT_CLASS(FSpotLight);

FSpotLight::FSpotLight(const FObjectInitializer& ObjectInitializer)
    : FLight(ObjectInitializer)
    , ConeAngle(45.0f)
{
}

void FSpotLight::SetConeAngle(float InConeAngle)
{
    ConeAngle = InConeAngle;
}
