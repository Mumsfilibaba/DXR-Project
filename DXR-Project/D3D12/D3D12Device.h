#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>

#include <memory>

#include <wrl/client.h>

#include "Types.h"

class D3D12Device
{
public:
	D3D12Device();
	~D3D12Device();

	bool Init(bool DebugEnable);

	ID3D12Device* GetDevice() const
	{
		return D3DDevice.Get();
	}

	IDXGIFactory2* GetFactory() const
	{
		return Factory.Get();
	}

	bool IsTearingSupported() const
	{
		return AllowTearing;
	}

	static D3D12Device* Create(bool DebugEnable);
	static D3D12Device* Get();

private:
	bool CreateFactory();
	bool ChooseAdapter();

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2>	Factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1>	Adapter;
	Microsoft::WRL::ComPtr<ID3D12Device>	D3DDevice;

	bool IsDebugEnabled		= false;
	BOOL AllowTearing		= FALSE;

	Uint32 AdapterID = 0;

	D3D_FEATURE_LEVEL MinFeatureLevel		= D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL ActiveFeatureLevel	= D3D_FEATURE_LEVEL_11_0;

	static std::unique_ptr<D3D12Device> Device;
};