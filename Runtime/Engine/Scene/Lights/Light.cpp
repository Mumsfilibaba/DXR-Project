#include "Light.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FLight

FLight::FLight()
    : Color()
    , ShadowNearPlane(1.0f)
    , ShadowFarPlane(30.0f)
    , ShadowBias(0.005f)
    , MaxShadowBias(0.05f)
{
    CORE_OBJECT_INIT();
}

void FLight::SetColor(const FVector3& InColor)
{
    Color = InColor;
}

void FLight::SetColor(float R, float G, float B)
{
    Color = FVector3(R, G, B);
}

void FLight::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}
