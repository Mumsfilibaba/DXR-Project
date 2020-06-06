#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>

#include <memory>

#include <wrl/client.h>

class D3D12GraphicsDevice
{
	D3D12GraphicsDevice(D3D12GraphicsDevice&& Other) = delete;
	D3D12GraphicsDevice(const D3D12GraphicsDevice& Other) = delete;

	D3D12GraphicsDevice& operator=(D3D12GraphicsDevice&& Other) = delete;
	D3D12GraphicsDevice& operator=(const D3D12GraphicsDevice& Other) = delete;

public:
	D3D12GraphicsDevice();
	~D3D12GraphicsDevice();

	bool Init();

	static D3D12GraphicsDevice* Create();
	static D3D12GraphicsDevice* Get();

private:
	bool CreateFactory();
	bool ChooseAdapter();

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2>	Factory = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter1>	Adapter = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device>	Device	= nullptr;

	static std::unique_ptr<D3D12GraphicsDevice> D3D12Device;
};