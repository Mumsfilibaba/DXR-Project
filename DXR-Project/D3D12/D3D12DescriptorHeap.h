#pragma once
#include "D3D12DeviceChild.h"

#include "Containers/TArray.h"

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
			VALIDATE(InHeap != nullptr);
			CPUStart = InHeap->GetCPUDescriptorHandleForHeapStart();

			FreeList.EmplaceBack(FirstRange);
		}

	public:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	Heap;
		D3D12_CPU_DESCRIPTOR_HANDLE						CPUStart;
		TArray<FreeRange>							FreeList;
	};

public:
	D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
	~D3D12OfflineDescriptorHeap();

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(Uint32& OutHeapIndex);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, Uint32 HeapIndex);

	virtual void SetDebugName(const std::string& InName) override;

private:
	void AllocateHeap();

private:
	TArray<DescriptorHeap> Heaps;
	std::wstring				DebugName;

	D3D12_DESCRIPTOR_HEAP_TYPE	Type;
	Uint32						DescriptorSize = 0;
};

/*
* D3D12OnlineDescriptorHeap
*/

class D3D12OnlineDescriptorHeap : public D3D12DeviceChild
{
public:
	D3D12OnlineDescriptorHeap(D3D12Device* InDevice, Uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);
	~D3D12OnlineDescriptorHeap();

	bool Initialize();
	
	Uint32 AllocateSlots(Uint32 NumSlots);

	virtual void SetDebugName(const std::string& InName) override;

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotAt(Uint32 Slot) const
	{
		return { CPUHeapStart.ptr + (Slot * DescriptorSize) };
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotAt(Uint32 Slot) const
	{
		return { GPUHeapStart.ptr + (Slot * DescriptorSize) };
	}

	FORCEINLINE Uint32 GetDescriptorSize() const
	{
		return DescriptorSize;
	}
	
	FORCEINLINE ID3D12DescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;

	D3D12_CPU_DESCRIPTOR_HANDLE	CPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE	GPUHeapStart;
	D3D12_DESCRIPTOR_HEAP_TYPE	Type;

	Uint32 DescriptorSize	= 0;
	Uint32 CurrentSlot		= 0;
	Uint32 DescriptorCount	= 0;
};

/*
* D3D12DescriptorTable
*/

class D3D12DescriptorTable
{
public:
	D3D12DescriptorTable(D3D12Device* InDevice, Uint32 InDescriptorCount);
	~D3D12DescriptorTable();

	void CopyDescriptors();
	
	void SetUnorderedAccessView(class D3D12UnorderedAccessView* View, Uint32 SlotIndex);
	void SetConstantBufferView(class D3D12ConstantBufferView* View, Uint32 SlotIndex);
	void SetShaderResourceView(class D3D12ShaderResourceView* View, Uint32 SlotIndex);

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUTableStartHandle() const
	{
		return CPUTableStart;
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUTableStartHandle() const
	{
		return GPUTableStart;
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUTableHandle(Uint32 DescriptorIndex) const
	{
		return { CPUTableStart.ptr + (DescriptorSize * DescriptorIndex) };
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUTableHandle(Uint32 DescriptorIndex) const
	{
		return { GPUTableStart.ptr + (DescriptorSize * DescriptorIndex) };
	}

private:
	D3D12Device* Device = nullptr;
	
	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> OfflineHandles;

	TUniquePtr<D3D12ShaderResourceView> NULLView;

	D3D12_CPU_DESCRIPTOR_HANDLE	CPUTableStart;
	D3D12_GPU_DESCRIPTOR_HANDLE	GPUTableStart;

	Uint32	DescriptorSize		= 0;
	Uint32	StartDescriptorSlot	= 0;
	Uint32	DescriptorCount		= 0;
	bool	IsDirty				= true;
};