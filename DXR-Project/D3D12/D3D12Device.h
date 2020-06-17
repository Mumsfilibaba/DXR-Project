#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>

#include <memory>

#include <wrl/client.h>

#include "Types.h"

class D3D12DescriptorHeap;

class D3D12Device
{
public:
	D3D12Device();
	~D3D12Device();

	bool Initialize(bool DebugEnable);

	ID3D12Device* GetDevice() const
	{
		return D3DDevice.Get();
	}

	ID3D12Device5* GetDXRDevice() const
	{
		return DXRDevice.Get();
	}

	IDXGIFactory2* GetFactory() const
	{
		return Factory.Get();
	}

	bool IsTearingSupported() const
	{
		return AllowTearing;
	}

	D3D12DescriptorHeap* GetGlobalResourceDescriptorHeap() const
	{
		return GlobalResourceDescriptorHeap;
	}

	static D3D12Device* Create(bool DebugEnable);

private:
	bool CreateFactory();
	bool ChooseAdapter();

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2>	Factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1>	Adapter;
	Microsoft::WRL::ComPtr<ID3D12Device>	D3DDevice;
	Microsoft::WRL::ComPtr<ID3D12Device5>	DXRDevice;

	bool DebugEnabled			= false;
	bool RayTracingSupported	= false;
	BOOL AllowTearing			= FALSE;

	Uint32 AdapterID = 0;

	D3D_FEATURE_LEVEL MinFeatureLevel		= D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL ActiveFeatureLevel	= D3D_FEATURE_LEVEL_11_0;

	D3D12DescriptorHeap* GlobalResourceDescriptorHeap = nullptr;
};