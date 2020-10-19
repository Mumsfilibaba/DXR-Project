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

	bool CreateSwapChain(IDXGIFactory2* Factory, const TSharedRef<WindowsWindow>& InWindow, D3D12CommandQueue* Queue);

	bool Resize(Uint32 InWidth, Uint32 InHeight);

	bool Present(Uint32 SyncInterval);

	Uint32 GetCurrentBackBufferIndex() const;

	FORCEINLINE void SetName(const std::string& Name)
	{
		SwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(Name.size()), Name.data());
	}

	FORCEINLINE D3D12Texture2D* GetBackBufferResource(Uint32 BackBufferIndex) const
	{
		return BackBuffers[BackBufferIndex].Get();
	}

	FORCEINLINE DXGI_FORMAT GetSurfaceFormat() const
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	FORCEINLINE Uint32 GetSurfaceCount() const
	{
		return 3;
	}

	FORCEINLINE Uint32 GetWidth() const
	{
		return Width;
	}

	FORCEINLINE Uint32 GetHeight() const
	{
		return Height;
	}

private:
	void RetriveSwapChainSurfaces();
	void ReleaseSurfaces();

private:
	TSharedRef<WindowsWindow> BeloningWindow;
	Microsoft::WRL::ComPtr<IDXGISwapChain3>		SwapChain;
	
	TArray<TSharedRef<D3D12Texture2D>>			BackBuffers;
	TArray<TSharedRef<D3D12RenderTargetView>>	BackBuffersViews;

	Uint32 Width	= 0;
	Uint32 Height	= 0;
	Uint32 Flags	= 0;
};