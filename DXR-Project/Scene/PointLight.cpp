#include "PointLight.h"

PointLight::PointLight()
	: Light()
	, Position(0.0f, 0.0f, 0.0f)
{
	CORE_OBJECT_INIT();
}

PointLight::~PointLight()
{
}

void PointLight::SetPosition(const XMFLOAT3& InPosition)
{
	Position = InPosition;
}

void PointLight::SetPosition(Float32 X, Float32 Y, Float32 Z)
{
	Position = XMFLOAT3(X, Y, Z);
}
