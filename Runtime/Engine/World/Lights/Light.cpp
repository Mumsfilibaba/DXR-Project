#include "Engine/World/Lights/Light.h"

FOBJECT_IMPLEMENT_CLASS(FLight);

FLight::FLight(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , Color()
    , Intensity(1.0f)
    , ShadowNearPlane(0.0f)
    , ShadowFarPlane(0.0f)
    , ShadowBias(0.005f)
{
}

FLight::FLight(const FObjectInitializer& ObjectInitializer, float InShadowNearPlane, float InShadowFarPlane)
    : FObject(ObjectInitializer)
    , Color()
    , Intensity(1.0f)
    , ShadowNearPlane(InShadowNearPlane)
    , ShadowFarPlane(InShadowFarPlane)
    , ShadowBias(0.005f)
{
}

FLight::~FLight()
{
}

void FLight::SetColor(const Vector3& InColor)
{
    Color = InColor;
}

void FLight::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}

void FLight::SetShadowNearPlane(float InShadowNearPlane)
{
    ShadowNearPlane = InShadowNearPlane;
}

void FLight::SetShadowFarPlane(float InShadowFarPlane)
{
    ShadowFarPlane = InShadowFarPlane;
}

void FLight::SetShadowBias(float InShadowBias)
{
    ShadowBias = InShadowBias;
}