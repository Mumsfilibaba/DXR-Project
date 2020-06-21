#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

class D3D12DescriptorHeap : public D3D12DeviceChild
{
public:
	D3D12DescriptorHeap(D3D12Device* InDevice);
	~D3D12DescriptorHeap();

	bool Initialize(D3D12_DESCRIPTOR_HEAP_TYPE Type, Uint32 DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(Uint32 DescriptorIndex) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(Uint32 DescriptorIndex) const;

	ID3D12DescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}

	Uint64 GetDescriptorSize() const
	{
		return DescriptorSize;
	}
	
public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	Uint64 DescriptorSize = 0;
};