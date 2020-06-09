#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

class D3D12Texture : public D3D12DeviceChild
{
	D3D12Texture(D3D12Texture&& Other)		= delete;
	D3D12Texture(const D3D12Texture& Other)	= delete;

	D3D12Texture& operator=(D3D12Texture&& Other)		= delete;
	D3D12Texture& operator=(const D3D12Texture& Other)	= delete;

public:
	D3D12Texture(D3D12Device* Device);
	~D3D12Texture();

	bool Init(DXGI_FORMAT Format, D3D12_RESOURCE_FLAGS Flags, Uint16 Width, Uint16 Height);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource1> Texture;
};