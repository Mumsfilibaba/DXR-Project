#pragma once
#include "D3D12Resource.h"

struct TextureProperties
{
	std::string					DebugName;
	DXGI_FORMAT					Format;
	D3D12_RESOURCE_FLAGS		Flags;
	Uint16						Width; 
	Uint16						Height;
	Uint16						ArrayCount;
	Uint16						MipLevels;
	D3D12_RESOURCE_STATES		InitalState;
	const D3D12_CLEAR_VALUE*	OptimizedClearValue;
	EMemoryType					MemoryType;
};

class D3D12RenderTargetView;
class D3D12DepthStencilView;

class D3D12Texture : public D3D12Resource
{
public:
	D3D12Texture(D3D12Device* InDevice);
	~D3D12Texture();

	bool Initialize(const TextureProperties& Properties);

	FORCEINLINE void SetRenderTargetView(TSharedPtr<D3D12RenderTargetView> InRenderTargetView)
	{
		RenderTargetView = InRenderTargetView;
	}

	FORCEINLINE TSharedPtr<D3D12RenderTargetView> GetRenderTargetView() const
	{
		return RenderTargetView;
	}

	FORCEINLINE void SetDepthStencilView(TSharedPtr<D3D12DepthStencilView> InDepthStencilView)
	{
		DepthStencilView = InDepthStencilView;
	}

	FORCEINLINE TSharedPtr<D3D12DepthStencilView> GetDepthStencilView() const
	{
		return DepthStencilView;
	}

protected:
	TSharedPtr<D3D12RenderTargetView> RenderTargetView;
	TSharedPtr<D3D12DepthStencilView> DepthStencilView;
};