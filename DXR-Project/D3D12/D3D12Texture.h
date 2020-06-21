#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

struct TextureProperties
{
	std::string				Name;
	DXGI_FORMAT				Format;
	D3D12_RESOURCE_FLAGS	Flags;
	Uint16					Width; 
	Uint16					Height;
	D3D12_HEAP_PROPERTIES	HeapProperties;
};

class D3D12Texture : public D3D12DeviceChild
{
public:
	D3D12Texture(D3D12Device* InDevice);
	~D3D12Texture();

	bool Initialize(const TextureProperties& InProperties);

	ID3D12Resource1* GetResource() const
	{
		return Texture.Get();
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource1> Texture;
};