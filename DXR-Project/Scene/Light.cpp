#include "Light.h"

#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Texture.h"

Light::Light()
	: Color()
{
}

Light::~Light()
{
	SAFEDELETE(LightBuffer);
	SAFEDELETE(ShadowMap);
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
