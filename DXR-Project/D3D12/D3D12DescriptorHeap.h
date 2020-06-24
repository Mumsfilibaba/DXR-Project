#pragma once
#include "D3D12Views.h"

class D3D12DescriptorHeap : public D3D12DeviceChild
{
public:
	D3D12DescriptorHeap(D3D12Device* InDevice);
	~D3D12DescriptorHeap();

	bool Initialize(D3D12_DESCRIPTOR_HEAP_TYPE Type, Uint32 DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

	D3D12DescriptorHandle Allocate();
	void Free(const D3D12DescriptorHandle& DescriptorHandle);

	FORCEINLINE ID3D12DescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}

	FORCEINLINE Uint64 GetDescriptorSize() const
	{
		return DescriptorSize;
	}
	
public:
	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	Heap;
	std::vector<D3D12DescriptorHandle>				FreeHandles;

	Uint64 DescriptorSize = 0;
};