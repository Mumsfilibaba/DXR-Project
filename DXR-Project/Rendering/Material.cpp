#include "Material.h"

Material::Material(const MaterialProperties& InProperties)
	: AlbedoMap(nullptr)
	, NormalMap(nullptr)
	, Roughness(nullptr)
	, Metallic(nullptr)
	, MaterialBuffer(nullptr)
	, Properties(InProperties)
{
}

Material::~Material()
{
}

void Material::Initialize(D3D12Device* Device)
{
}
