#pragma once
#include "Windows/WindowsWindow.h"

#include "D3D12Texture.h"
#include "D3D12Views.h"
#include "D3D12RefCountedObject.h"

#include <dxgi1_6.h>

class WindowsWindow;
class D3D12CommandQueue;

/*
* D3D12SwapChain
*/

class D3D12SwapChain : public D3D12RefCountedObject
{
public:
	D3D12SwapChain(D3D12Device* InDevice);
	~D3D12SwapChain();

	bool CreateSwapChain(IDXGIFactory2* Factory, const TSharedRef<WindowsWindow>& InWindow, D3D12CommandQueue* Queue);

	bool Resize(UInt32 InWidth, UInt32 InHeight);

	bool Present(UInt32 SyncInterval);

	UInt32 GetCurrentBackBufferIndex() const;

	FORCEINLINE void SetName(const std::string& Name)
	{
		SwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(Name.size()), Name.data());
	}

	FORCEINLINE DXGI_FORMAT GetSurfaceFormat() const
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	FORCEINLINE UInt32 GetSurfaceCount() const
	{
		return 3;
	}

	FORCEINLINE UInt32 GetWidth() const
	{
		return Width;
	}

	FORCEINLINE UInt32 GetHeight() const
	{
		return Height;
	}

private:
	TSharedRef<WindowsWindow> Window;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> SwapChain;

	UInt32 Width	= 0;
	UInt32 Height	= 0;
	UInt32 Flags	= 0;
};