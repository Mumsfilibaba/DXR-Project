#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>

#include <memory>

#include <wrl/client.h>

#include "../Types.h"

class D3D12GraphicsDevice
{
	D3D12GraphicsDevice(D3D12GraphicsDevice&& Other)		= delete;
	D3D12GraphicsDevice(const D3D12GraphicsDevice& Other)	= delete;

	D3D12GraphicsDevice& operator=(D3D12GraphicsDevice&& Other)			= delete;
	D3D12GraphicsDevice& operator=(const D3D12GraphicsDevice& Other)	= delete;

public:
	D3D12GraphicsDevice();
	~D3D12GraphicsDevice();

	bool Init(bool DebugEnable);

	ID3D12Device* GetDevice() const
	{
		return Device.Get();
	}

	IDXGIFactory2* GetFactory() const
	{
		return Factory.Get();
	}

	static D3D12GraphicsDevice* Create(bool DebugEnable);
	static D3D12GraphicsDevice* Get();

private:
	bool CreateFactory();
	bool ChooseAdapter();

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2>	Factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1>	Adapter;
	Microsoft::WRL::ComPtr<ID3D12Device>	Device;

	bool IsDebugEnabled		= false;
	bool IsTearingSupported = false;

	Uint32 AdapterID = 0;

	D3D_FEATURE_LEVEL MinFeatureLevel		= D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL ActiveFeatureLevel	= D3D_FEATURE_LEVEL_11_0;

	static std::unique_ptr<D3D12GraphicsDevice> D3D12Device;
};