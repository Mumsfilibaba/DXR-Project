#include "SpotLight.h"

FSpotLight::FSpotLight()
    : FLight()
    , ConeAngle(45.0f)
{
    FOBJECT_INIT();
}

void FSpotLight::SetConeAngle(float InConeAngle)
{
    ConeAngle = InConeAngle;
}
