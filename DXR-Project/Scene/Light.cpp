#include "Light.h"

Light::Light()
	: Color()
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
