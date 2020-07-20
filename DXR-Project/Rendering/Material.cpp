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
	SAFEDELETE(DescriptorTable);
}

void Material::Initialize(D3D12Device* Device)
{
	VALIDATE(AlbedoMap != nullptr);
	VALIDATE(NormalMap != nullptr);

	DescriptorTable = new D3D12DescriptorTable(Device, 2);
	DescriptorTable->SetShaderResourceView(AlbedoMap->GetShaderResourceView(0).get(), 0);
	DescriptorTable->SetShaderResourceView(NormalMap->GetShaderResourceView(0).get(), 1);
	DescriptorTable->CopyDescriptors();
}
