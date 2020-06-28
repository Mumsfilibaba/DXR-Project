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