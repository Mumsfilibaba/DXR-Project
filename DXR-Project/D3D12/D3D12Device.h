#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>

#include "Types.h"

class D3D12DescriptorHeap;
class D3D12OfflineDescriptorHeap;
class D3D12OnlineDescriptorHeap;
class D3D12ComputePipelineState;
class D3D12RootSignature;

class D3D12Device
{
public:
	D3D12Device();
	~D3D12Device();

	bool Initialize(bool DebugEnable);

	std::string GetAdapterName() const;

	FORCEINLINE ID3D12Device* GetDevice() const
	{
		return D3DDevice.Get();
	}

	FORCEINLINE ID3D12Device5* GetDXRDevice() const
	{
		return DXRDevice.Get();
	}

	FORCEINLINE IDXGIFactory2* GetFactory() const
	{
		return Factory.Get();
	}

	FORCEINLINE bool IsTearingSupported() const
	{
		return AllowTearing;
	}

	FORCEINLINE bool IsRayTracingSupported() const
	{
		return false;// RayTracingSupported;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalResourceDescriptorHeap() const
	{
		return GlobalResourceDescriptorHeap;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalRenderTargetDescriptorHeap() const
	{
		return GlobalRenderTargetDescriptorHeap;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalDepthStencilDescriptorHeap() const
	{
		return GlobalDepthStencilDescriptorHeap;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalSamplerDescriptorHeap() const
	{
		return GlobalSamplerDescriptorHeap;
	}

	FORCEINLINE D3D12OnlineDescriptorHeap* GetGlobalOnlineResourceHeap() const
	{
		return GlobalOnlineResourceHeap;
	}

	static D3D12Device* Make(bool DebugEnable);

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

	D3D12OfflineDescriptorHeap* GlobalResourceDescriptorHeap		= nullptr;
	D3D12OfflineDescriptorHeap* GlobalRenderTargetDescriptorHeap	= nullptr;
	D3D12OfflineDescriptorHeap* GlobalDepthStencilDescriptorHeap	= nullptr;
	D3D12OfflineDescriptorHeap* GlobalSamplerDescriptorHeap			= nullptr;

	D3D12OnlineDescriptorHeap* GlobalOnlineResourceHeap = nullptr;
};