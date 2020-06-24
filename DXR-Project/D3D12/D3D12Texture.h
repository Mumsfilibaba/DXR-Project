#pragma once
#include "D3D12Resource.h"

class D3D12ShaderResourceView;
class D3D12UnorderedAccessView;

struct TextureProperties
{
	std::string				Name;
	DXGI_FORMAT				Format;
	D3D12_RESOURCE_FLAGS	Flags;
	Uint16					Width; 
	Uint16					Height;
	D3D12_RESOURCE_STATES	InitalState;
	EMemoryType				MemoryType;
};

class D3D12Texture : public D3D12Resource
{
public:
	D3D12Texture(D3D12Device* InDevice);
	~D3D12Texture();

	bool Initialize(const TextureProperties& Properties);

	FORCEINLINE void SetShaderResourceView(std::shared_ptr<D3D12ShaderResourceView>& InShaderResourceView)
	{
		ShaderResourceView = InShaderResourceView;
	}

	FORCEINLINE std::shared_ptr<D3D12ShaderResourceView> GetShaderResourceView() const
	{
		return ShaderResourceView;
	}

	FORCEINLINE void SetUnorderedAccessView(std::shared_ptr<D3D12UnorderedAccessView>& InUnorderedAccessView)
	{
		UnorderedAccessView = InUnorderedAccessView;
	}

	FORCEINLINE std::shared_ptr<D3D12UnorderedAccessView> GetUnorderedAccessView() const
	{
		return UnorderedAccessView;
	}

private:
	std::shared_ptr<D3D12ShaderResourceView> ShaderResourceView;
	std::shared_ptr<D3D12UnorderedAccessView> UnorderedAccessView;
};