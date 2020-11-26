#pragma once
#include "D3D12Resource.h"

/*
* TextureProperties
*/

struct TextureProperties
{
	std::string					DebugName;
	DXGI_FORMAT					Format;
	D3D12_RESOURCE_FLAGS		Flags;
	uint16						Width; 
	uint16						Height;
	uint16						ArrayCount;
	uint16						MipLevels;
	uint32						SampleCount;
	D3D12_RESOURCE_STATES		InitalState;
	const D3D12_CLEAR_VALUE*	OptimizedClearValue;
	EMemoryType					MemoryType;
};

class D3D12RenderTargetView;
class D3D12DepthStencilView;

/*
* D3D12Texture
*/

class D3D12Texture : public D3D12Resource
{
public:
	D3D12Texture(D3D12Device* InDevice);
	~D3D12Texture();

	bool Initialize(const TextureProperties& Properties);

	void SetRenderTargetView(TSharedPtr<D3D12RenderTargetView> InRenderTargetView, uint32 SubresourceIndex);
	void SetDepthStencilView(TSharedPtr<D3D12DepthStencilView> InDepthStencilView, uint32 SubresourceIndex);

	FORCEINLINE TSharedPtr<D3D12RenderTargetView> GetRenderTargetView(uint32 Index) const
	{
		return RenderTargetViews[Index];
	}

	FORCEINLINE TSharedPtr<D3D12DepthStencilView> GetDepthStencilView(uint32 Index) const
	{
		return DepthStencilViews[Index];
	}

protected:
	TArray<TSharedPtr<D3D12RenderTargetView>> RenderTargetViews;
	TArray<TSharedPtr<D3D12DepthStencilView>> DepthStencilViews;
};