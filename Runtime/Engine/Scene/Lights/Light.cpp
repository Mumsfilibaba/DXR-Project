#include "Light.h"

FLight::FLight()
    : Color()
    , ShadowNearPlane(1.0f)
    , ShadowFarPlane(30.0f)
    , ShadowBias(0.005f)
    , MaxShadowBias(0.05f)
{
    FOBJECT_INIT();
}

void FLight::SetColor(const FVector3& InColor)
{
    Color = InColor;
}

void FLight::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}
