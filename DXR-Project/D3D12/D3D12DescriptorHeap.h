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
		TArray<FreeRange>								FreeList;
	};

public:
	D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
	~D3D12OfflineDescriptorHeap();

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(UInt32& OutHeapIndex);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, UInt32 HeapIndex);

	void SetName(const std::string& InName);

	FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
	{
		return Type;
	}

	FORCEINLINE Uint32 GetDescriptorSize() const
	{
		return DescriptorSize;
	}

private:
	void AllocateHeap();

private:
	TArray<DescriptorHeap> Heaps;
	std::wstring Name;

	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	Uint32 DescriptorSize = 0;
};

/*
* D3D12OnlineDescriptorHeap
*/

class D3D12OnlineDescriptorHeap : public D3D12DeviceChild
{
public:
	D3D12OnlineDescriptorHeap(D3D12Device* InDevice, UInt32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);
	~D3D12OnlineDescriptorHeap();

	bool Initialize();
	
	UInt32 AllocateSlots(UInt32 NumSlots);

	FORCEINLINE void SetName(const std::string& InName)
	{
		std::wstring WideName = ConvertToWide(InName);
		Heap->SetName(WideName.c_str());
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotAt(UInt32 Slot) const
	{
		return { CPUHeapStart.ptr + (Slot * DescriptorSize) };
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotAt(UInt32 Slot) const
	{
		return { GPUHeapStart.ptr + (Slot * DescriptorSize) };
	}

	FORCEINLINE UInt32 GetDescriptorSize() const
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

	UInt32 DescriptorSize	= 0;
	UInt32 CurrentSlot		= 0;
	UInt32 DescriptorCount	= 0;
};

/*
* D3D12DescriptorTable
*/

class D3D12DescriptorTable
{
public:
	D3D12DescriptorTable(D3D12Device* InDevice, UInt32 InDescriptorCount);
	~D3D12DescriptorTable();

	void CopyDescriptors();
	
	void SetUnorderedAccessView(class D3D12UnorderedAccessView* View, UInt32 SlotIndex);
	void SetConstantBufferView(class D3D12ConstantBufferView* View, UInt32 SlotIndex);
	void SetShaderResourceView(class D3D12ShaderResourceView* View, UInt32 SlotIndex);

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUTableStartHandle() const
	{
		return CPUTableStart;
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUTableStartHandle() const
	{
		return GPUTableStart;
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUTableHandle(UInt32 DescriptorIndex) const
	{
		return { CPUTableStart.ptr + (DescriptorSize * DescriptorIndex) };
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUTableHandle(UInt32 DescriptorIndex) const
	{
		return { GPUTableStart.ptr + (DescriptorSize * DescriptorIndex) };
	}

private:
	D3D12Device* Device = nullptr;
	
	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> OfflineHandles;

	TUniquePtr<D3D12ShaderResourceView> NULLView;

	D3D12_CPU_DESCRIPTOR_HANDLE	CPUTableStart;
	D3D12_GPU_DESCRIPTOR_HANDLE	GPUTableStart;

	UInt32	DescriptorSize		= 0;
	UInt32	StartDescriptorSlot	= 0;
	UInt32	DescriptorCount		= 0;
	bool	IsDirty				= true;
};