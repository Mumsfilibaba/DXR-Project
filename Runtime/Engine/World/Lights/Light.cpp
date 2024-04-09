#include "Light.h"

FOBJECT_IMPLEMENT_CLASS(FLight);

FLight::FLight(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , Color()
    , ShadowNearPlane(1.0f)
    , ShadowFarPlane(30.0f)
    , ShadowBias(0.005f)
    , MaxShadowBias(0.05f)
{
}

void FLight::SetColor(const FVector3& InColor)
{
    Color = InColor;
}

void FLight::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}
