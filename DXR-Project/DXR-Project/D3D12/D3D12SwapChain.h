#pragma once
#include <dxgi1_6.h>

#include <wrl/client.h>

class D3D12GraphicsDevice;
class WindowsWindow;
class D3D12CommandQueue;

class D3D12SwapChain
{
	D3D12SwapChain(D3D12SwapChain&& Other)		= delete;
	D3D12SwapChain(const D3D12SwapChain& Other)	= delete;

	D3D12SwapChain& operator=(D3D12SwapChain&& Other)		= delete;
	D3D12SwapChain& operator=(const D3D12SwapChain& Other)	= delete;

public:
	D3D12SwapChain(D3D12GraphicsDevice* Device);
	~D3D12SwapChain();

	bool Init(WindowsWindow* Window, D3D12CommandQueue* Queue);

private:
	D3D12GraphicsDevice*					Device = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain1>	SwapChain;
};