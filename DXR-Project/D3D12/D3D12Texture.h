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
	UInt16						Width; 
	UInt16						Height;
	UInt16						ArrayCount;
	UInt16						MipLevels;
	UInt32						SampleCount;
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

	void SetRenderTargetView(TSharedPtr<D3D12RenderTargetView> InRenderTargetView, UInt32 SubresourceIndex);
	void SetDepthStencilView(TSharedPtr<D3D12DepthStencilView> InDepthStencilView, UInt32 SubresourceIndex);

	FORCEINLINE TSharedPtr<D3D12RenderTargetView> GetRenderTargetView(UInt32 Index) const
	{
		return RenderTargetViews[Index];
	}

	FORCEINLINE TSharedPtr<D3D12DepthStencilView> GetDepthStencilView(UInt32 Index) const
	{
		return DepthStencilViews[Index];
	}

protected:
	TArray<TSharedPtr<D3D12RenderTargetView>> RenderTargetViews;
	TArray<TSharedPtr<D3D12DepthStencilView>> DepthStencilViews;
};