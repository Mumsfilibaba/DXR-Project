#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

#include <dxgi1_6.h>

#include <vector>

class WindowsWindow;
class D3D12CommandQueue;

class D3D12SwapChain : public D3D12DeviceChild
{
	D3D12SwapChain(D3D12SwapChain&& Other)		= delete;
	D3D12SwapChain(const D3D12SwapChain& Other)	= delete;

	D3D12SwapChain& operator=(D3D12SwapChain&& Other)		= delete;
	D3D12SwapChain& operator=(const D3D12SwapChain& Other)	= delete;

public:
	D3D12SwapChain(D3D12Device* Device);
	~D3D12SwapChain();

	bool Init(WindowsWindow* Window, D3D12CommandQueue* Queue);

	bool Present(Uint32 SyncInterval);

	Uint32 GetCurrentBackBufferIndex() const;

	ID3D12Resource* GetSurfaceResource(Uint32 SurfaceIndex) const
	{
		return BackBuffers[SurfaceIndex].Get();
	}

	DXGI_FORMAT GetSurfaceFormat() const
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	Uint32 GetSurfaceCount() const
	{
		return 3;
	}

private:
	void RetriveSwapChainSurfaces();

private:
	Microsoft::WRL::ComPtr<IDXGISwapChain3>				SwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> BackBuffers;
};