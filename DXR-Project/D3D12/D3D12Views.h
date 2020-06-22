#pragma once
#include "Defines.h"

#include <d3d12.h>

class D3D12DescriptorHandle
{
public:
	D3D12DescriptorHandle();
	~D3D12DescriptorHandle();

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const
	{
		return CPUHandle;
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const
	{
		return GPUHandle;
	}

private:
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
};

class D3D12View
{
protected:
	D3D12View()		= default;
	~D3D12View()	= default;

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource>	Resource;
};

class D3D12ShaderResourceView : public D3D12View
{
public:
	D3D12ShaderResourceView();
	~D3D12ShaderResourceView();
};