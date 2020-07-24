#pragma once
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12DescriptorHeap.h"

struct MaterialProperties
{
	Float32 Metallic	= 0.0f;
	Float32 Roughness	= 0.0f;
	Float32 AO			= 1.0f;
};

/*
* Class for Material
*/

class Material
{
public:
	Material(const MaterialProperties& InProperties);
	~Material();

	void Initialize(D3D12Device* Device);

	FORCEINLINE D3D12DescriptorTable* GetDescriptorTable() const
	{
		return DescriptorTable;
	}

	FORCEINLINE const MaterialProperties& GetMaterialProperties() const 
	{
		return Properties;
	}

public:
	std::shared_ptr<D3D12Texture>	AlbedoMap;
	std::shared_ptr<D3D12Texture>	NormalMap;
	std::shared_ptr<D3D12Texture>	Roughness;
	std::shared_ptr<D3D12Texture>	Height;
	std::shared_ptr<D3D12Texture>	AO;
	std::shared_ptr<D3D12Texture>	Metallic;

private:
	MaterialProperties		Properties;
	D3D12Buffer*			MaterialBuffer	= nullptr;
	D3D12DescriptorTable*	DescriptorTable = nullptr;
};