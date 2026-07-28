#include "Engine/World/Components/LightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FLightComponent);

FLightComponent::FLightComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Color()
    , Intensity(1.0f)
    , ShadowNearPlane(0.0f)
    , ShadowFarPlane(0.0f)
    , ShadowBias(0.005f)
{
}

FLightComponent::FLightComponent(
    const FObjectInitializer& ObjectInitializer,
    float InShadowNearPlane,
    float InShadowFarPlane)
    : FSceneComponent(ObjectInitializer)
    , Color()
    , Intensity(1.0f)
    , ShadowNearPlane(InShadowNearPlane)
    , ShadowFarPlane(InShadowFarPlane)
    , ShadowBias(0.005f)
{
}

FLightComponent::~FLightComponent()
{
}

void FLightComponent::SetColor(const Vector3& InColor)
{
    Color = InColor;
}

void FLightComponent::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}

void FLightComponent::SetShadowNearPlane(float InShadowNearPlane)
{
    ShadowNearPlane = InShadowNearPlane;
}

void FLightComponent::SetShadowFarPlane(float InShadowFarPlane)
{
    ShadowFarPlane = InShadowFarPlane;
}

void FLightComponent::SetShadowBias(float InShadowBias)
{
    ShadowBias = InShadowBias;
}
