#include "SpotLight.h"

SpotLight::SpotLight()
    : Light()
    , ConeAngle(45.0f)
{
    CORE_OBJECT_INIT();
}

SpotLight::~SpotLight()
{
}

void SpotLight::SetConeAngle(float InConeAngle)
{
    ConeAngle = InConeAngle;
}
