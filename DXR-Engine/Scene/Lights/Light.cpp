#include "Light.h"

Light::Light()
    : BaseLight()
    , Color()
    , ShadowBias(0.005f)
    , MaxShadowBias(0.05f)
    , ShadowNearPlane(1.0f)
    , ShadowFarPlane(30.0f)
{
    CORE_OBJECT_INIT();
}

void Light::SetColor(const XMFLOAT3& InColor)
{
    Color = InColor;
}

void Light::SetColor(Float R, Float G, Float B)
{
    Color = XMFLOAT3(R, G, B);
}

void Light::SetIntensity(Float InIntensity)
{
    Intensity = InIntensity;
}
