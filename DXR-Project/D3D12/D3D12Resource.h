#pragma once
#include "RenderingCore/Buffer.h"

#include "D3D12DeviceChild.h"

/*
* D3D12Resource
*/

class D3D12Resource : public D3D12DeviceChild
{
	friend class D3D12RenderingAPI;

public:
	D3D12Resource(D3D12Device* InDevice);
	virtual ~D3D12Resource();

	Void* Map(const Range* MappedRange);
	void Unmap(const Range* WrittenRange);

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return D3DResource.Get();
	}

	FORCEINLINE D3D12_HEAP_TYPE GetHeapType() const
	{
		return HeapType;
	}

	FORCEINLINE D3D12_RESOURCE_STATES GetResourceState() const
	{
		return ResourceState;
	}

	FORCEINLINE const D3D12_RESOURCE_DESC& GetDesc() const
	{
		return Desc;
	}

	FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
	{
		return Address;
	}

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource;

	D3D12_HEAP_TYPE				HeapType;
	D3D12_RESOURCE_STATES		ResourceState;
	D3D12_RESOURCE_DESC			Desc;
	D3D12_GPU_VIRTUAL_ADDRESS	Address;
};