#pragma once
#include "D3D12Texture.h"
#include "D3D12Views.h"

#include <dxgi1_6.h>

class WindowsWindow;
class D3D12CommandQueue;

/*
* D3D12SwapChain
*/

class D3D12SwapChain : public D3D12DeviceChild
{
public:
	D3D12SwapChain(D3D12Device* InDevice);
	~D3D12SwapChain();

	bool Initialize(WindowsWindow* Window, D3D12CommandQueue* Queue);

	bool Resize(uint32 InWidth, uint32 InHeight);

	bool Present(uint32 SyncInterval);

	uint32 GetCurrentBackBufferIndex() const;

	FORCEINLINE D3D12Texture* GetSurfaceResource(uint32 SurfaceIndex) const
	{
		return BackBuffers[SurfaceIndex].Get();
	}

	FORCEINLINE DXGI_FORMAT GetSurfaceFormat() const
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	FORCEINLINE uint32 GetSurfaceCount() const
	{
		return 3;
	}

	FORCEINLINE uint32 GetWidth() const
	{
		return Width;
	}

	FORCEINLINE uint32 GetHeight() const
	{
		return Height;
	}

public:
	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

private:
	void RetriveSwapChainSurfaces();
	void ReleaseSurfaces();

private:
	Microsoft::WRL::ComPtr<IDXGISwapChain3>		SwapChain;
	
	TArray<TSharedPtr<D3D12Texture>>			BackBuffers;
	TArray<TSharedPtr<D3D12RenderTargetView>>	BackBuffersViews;

	uint32 Width	= 0;
	uint32 Height	= 0;
	uint32 Flags	= 0;
};