#include "SpotLight.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FSpotLight

FSpotLight::FSpotLight()
    : FLight()
    , ConeAngle(45.0f)
{
    CORE_OBJECT_INIT();
}

void FSpotLight::SetConeAngle(float InConeAngle)
{
    ConeAngle = InConeAngle;
}
