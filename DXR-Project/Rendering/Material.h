#pragma once
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Texture.h"

struct MaterialProperties
{
	Float32 Metallic	= 0.0f;
	Float32 Roughness	= 0.0f;
	Float32 AO			= 1.0f;
};

class Material
{
public:
	Material(const MaterialProperties& InProperties);
	~Material();

	void Initialize(D3D12Device* Device);

public:
	std::shared_ptr<D3D12Texture>	AlbedoMap;
	std::shared_ptr<D3D12Texture>	NormalMap;
	std::shared_ptr<D3D12Texture>	Roughness;
	std::shared_ptr<D3D12Texture>	Metallic;
	MaterialProperties Properties;

private:
	std::shared_ptr<D3D12Buffer>	MaterialBuffer;
};