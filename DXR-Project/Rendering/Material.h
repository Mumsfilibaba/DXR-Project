#pragma once
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Texture.h"

class Material
{
public:
	Material();
	~Material();

private:
	std::shared_ptr<D3D12Texture>	AlbedoMap;
	std::shared_ptr<D3D12Texture>	NormalMap;
	std::shared_ptr<D3D12Texture>	Roughness;
	std::shared_ptr<D3D12Texture>	Metallic;
	std::shared_ptr<D3D12Buffer>	MaterialBuffer;
};