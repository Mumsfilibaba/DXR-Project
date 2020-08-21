#include "Light.h"

#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12DescriptorHeap.h"

Light::Light()
	: Color()
{
	CORE_OBJECT_INIT();
}

Light::~Light()
{
	SAFEDELETE(LightBuffer);
	SAFEDELETE(ShadowMap);
	SAFEDELETE(DescriptorTable);
}

void Light::SetColor(const XMFLOAT3& InColor)
{
	Color = InColor;
	LightBufferIsDirty = true;
}

void Light::SetColor(Float32 R, Float32 G, Float32 B)
{
	Color = XMFLOAT3(R, G, B);
	LightBufferIsDirty = true;
}

void Light::SetIntensity(Float32 InIntensity)
{
	Intensity = InIntensity;
	LightBufferIsDirty = true;
}
