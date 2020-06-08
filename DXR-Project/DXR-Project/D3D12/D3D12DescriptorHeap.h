#pragma once
#include "D3D12DeviceChild.h"

#include "../Types.h"

class D3D12DescriptorHeap : public D3D12DeviceChild
{
	D3D12DescriptorHeap(D3D12DescriptorHeap&& Other)		= delete;
	D3D12DescriptorHeap(const D3D12DescriptorHeap& Other)	= delete;

	D3D12DescriptorHeap& operator=(D3D12DescriptorHeap&& Other)			= delete;
	D3D12DescriptorHeap& operator=(const D3D12DescriptorHeap& Other)	= delete;

public:
	D3D12DescriptorHeap(D3D12Device* Device);
	~D3D12DescriptorHeap();

	bool Init(D3D12_DESCRIPTOR_HEAP_TYPE Type, Uint32 DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(Uint32 DescriptorIndex) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(Uint32 DescriptorIndex) const;

	Uint64 GetDescriptorSize() const
	{
		return DescriptorSize;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	Uint64 DescriptorSize = 0;
};