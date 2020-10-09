#pragma once
#include "D3D12DeviceChild.h"

/*
* D3D12Resource
*/

class D3D12Resource : public D3D12DeviceChild
{
public:
	D3D12Resource(D3D12Device* InDevice);
	virtual ~D3D12Resource();

	bool Initialize(const D3D12_RESOURCE_DESC& InDesc, D3D12_RESOURCE_STATES InitalState, D3D12_HEAP_TYPE InHeapType, const D3D12_CLEAR_VALUE* OptimizedClearValue);

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource;
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
	ComRef<ID3D12Resource> Resource;

	D3D12_HEAP_TYPE				HeapType;
	D3D12_RESOURCE_STATES		ResourceState;
	D3D12_RESOURCE_DESC			Desc;
	D3D12_GPU_VIRTUAL_ADDRESS	Address;
};