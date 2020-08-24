#include "Light.h"

Light::Light()
	: Color()
	, ShadowBias(0.005f)
	, MaxShadowBias(0.05f)
	, ShadowNearPlane(1.0f)
	, ShadowFarPlane(30.0f)
{
	CORE_OBJECT_INIT();
}

Light::~Light()
{
}

void Light::SetColor(const XMFLOAT3& InColor)
{
	Color = InColor;
}

void Light::SetColor(Float32 R, Float32 G, Float32 B)
{
	Color = XMFLOAT3(R, G, B);
}

void Light::SetIntensity(Float32 InIntensity)
{
	Intensity = InIntensity;
}
