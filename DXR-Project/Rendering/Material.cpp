#include "Material.h"

Material::Material(const MaterialProperties& InProperties)
	: AlbedoMap(nullptr)
	, NormalMap(nullptr)
	, Roughness(nullptr)
	, Metallic(nullptr)
	, AO(nullptr)
	, Height(nullptr)
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
	VALIDATE(AlbedoMap	!= nullptr);
	VALIDATE(NormalMap	!= nullptr);
	VALIDATE(Roughness	!= nullptr);
	VALIDATE(Height		!= nullptr);
	VALIDATE(AO			!= nullptr);
	VALIDATE(Metallic	!= nullptr);

	DescriptorTable = new D3D12DescriptorTable(Device, 6);
	DescriptorTable->SetShaderResourceView(AlbedoMap->GetShaderResourceView(0).get(), 0);
	DescriptorTable->SetShaderResourceView(NormalMap->GetShaderResourceView(0).get(), 1);
	DescriptorTable->SetShaderResourceView(Roughness->GetShaderResourceView(0).get(), 2);
	DescriptorTable->SetShaderResourceView(Height->GetShaderResourceView(0).get(), 3);
	DescriptorTable->SetShaderResourceView(Metallic->GetShaderResourceView(0).get(), 4);
	DescriptorTable->SetShaderResourceView(AO->GetShaderResourceView(0).get(), 5);
	DescriptorTable->CopyDescriptors();
}
