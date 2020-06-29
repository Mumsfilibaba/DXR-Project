#pragma once
#include "D3D12DeviceChild.h"

/*
* D3D12OfflineDescriptorHeap
*/

class D3D12OfflineDescriptorHeap : public D3D12DeviceChild
{
	struct FreeRange
	{
	public:
		FreeRange()
			: Begin({ 0 })
			, End({ 0 })
		{
		}

		FreeRange(D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
			: Begin(InBegin)
			, End(InEnd)
		{
		}

		bool IsValid() const
		{
			return Begin.ptr < End.ptr;
		}

	public:
		D3D12_CPU_DESCRIPTOR_HANDLE Begin;
		D3D12_CPU_DESCRIPTOR_HANDLE End;
	};

	struct DescriptorHeap
	{
	public:
		DescriptorHeap()
			: Heap(nullptr)
			, FreeList()
		{
		}

		DescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& InHeap, FreeRange FirstRange)
			: Heap(InHeap)
			, FreeList()
		{
			FreeList.emplace_back(FirstRange);
		}

	public:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	Heap;
		std::vector<FreeRange>							FreeList;
	};

public:
	D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
	~D3D12OfflineDescriptorHeap();

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(Uint32& OutHeapIndex);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, Uint32 HeapIndex);

	virtual void SetName(const std::string& InName) override;

private:
	void AllocateHeap();

private:
	std::vector<DescriptorHeap> Heaps;
	std::wstring				DebugName;

	D3D12_DESCRIPTOR_HEAP_TYPE	Type;
	Uint32						DescriptorSize = 0;
};

/*
* D3D12DescriptorTable
*/

class D3D12DescriptorTable
{
public:
	D3D12DescriptorTable(D3D12Device* InDevice);
	~D3D12DescriptorTable();

	void SetViewAtSlot(D3D12View* View, Uint32 SlotIndex);

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetTableStartHandle() const
	{
		return TableStart;
	}

private:
	D3D12Device* Device = nullptr;
	
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> CpuHandles;
	D3D12_GPU_DESCRIPTOR_HANDLE TableStart;
};