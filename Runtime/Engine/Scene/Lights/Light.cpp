#include "Light.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CLight

CLight::CLight()
    : Color()
    , ShadowNearPlane(1.0f)
    , ShadowFarPlane(30.0f)
    , ShadowBias(0.005f)
    , MaxShadowBias(0.05f)
{
    CORE_OBJECT_INIT();
}

void CLight::SetColor(const CVector3& InColor)
{
    Color = InColor;
}

void CLight::SetColor(float R, float G, float B)
{
    Color = CVector3(R, G, B);
}

void CLight::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}
