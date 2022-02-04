#include "SpotLight.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CSpotLight

CSpotLight::CSpotLight()
    : CLight()
    , ConeAngle(45.0f)
{
    CORE_OBJECT_INIT();
}

CSpotLight::~CSpotLight()
{
}

void CSpotLight::SetConeAngle(float InConeAngle)
{
    ConeAngle = InConeAngle;
}
