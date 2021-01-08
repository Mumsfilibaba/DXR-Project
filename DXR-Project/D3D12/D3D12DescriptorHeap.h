#pragma once
#include <Containers/TArray.h>

#include "Utilities/StringUtilities.h"

#include "D3D12RefCountedObject.h"
#include "D3D12Device.h"

/*
* D3D12DescriptorHeap
*/

class D3D12DescriptorHeap : public D3D12RefCountedObject
{
public:
	inline D3D12DescriptorHeap(D3D12Device* InDevice, ID3D12DescriptorHeap* InHeap)
		: D3D12RefCountedObject(InDevice)
		, Heap(InHeap)
		, CPUStart({ 0 })
		, GPUStart({ 0 })
		, DescriptorHandleIncrementSize(0)
		, Type()
	{
		VALIDATE(Heap != nullptr);
		
		CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
		GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();

		D3D12_DESCRIPTOR_HEAP_DESC Desc = Heap->GetDesc();
		Type			= Desc.Type;
		NumDescriptors	= Desc.NumDescriptors;
		DescriptorHandleIncrementSize = InDevice->GetDescriptorHandleIncrementSize(Type);
	}

	FORCEINLINE void SetName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		Heap->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12DescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const
	{
		return CPUStart;
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const
	{
		return GPUStart;
	}

	FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
	{
		return Type;
	}

	FORCEINLINE UInt32 GetNumDescriptors() const
	{
		return NumDescriptors;
	}

	FORCEINLINE UInt32 GetDescriptorHandleIncrementSize() const
	{
		return DescriptorHandleIncrementSize;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUStart;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUStart;
	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	UInt32 DescriptorHandleIncrementSize;
	UInt32 NumDescriptors;
};

/*
* D3D12OfflineDescriptorHeap
*/

class D3D12OfflineDescriptorHeap : public D3D12RefCountedObject
{
	/*
	* DescriptorRange
	*/

	struct DescriptorRange
	{
		DescriptorRange() = default;

		DescriptorRange(D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
			: Begin(InBegin)
			, End(InEnd)
		{
		}

		FORCEINLINE bool IsValid() const
		{
			return Begin.ptr < End.ptr;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Begin	= { 0 };
		D3D12_CPU_DESCRIPTOR_HANDLE End		= { 0 };
	};

	/*
	* DescriptorHeap
	*/

	struct DescriptorHeap
	{
		inline DescriptorHeap(TSharedRef<D3D12DescriptorHeap>& InHeap)
			: FreeList()
			, Heap(InHeap)
		{
			DescriptorRange WholeRange;
			WholeRange.Begin	= Heap->GetCPUDescriptorHandleForHeapStart();
			WholeRange.End.ptr	= WholeRange.Begin.ptr + (Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors());
			FreeList.EmplaceBack(WholeRange);
		}

		TArray<DescriptorRange> FreeList;
		TSharedRef<D3D12DescriptorHeap> Heap;
	};

public:
	D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(UInt32& OutHeapIndex);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, UInt32 HeapIndex);

	void SetName(const std::string& InName);

	FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
	{
		return Type;
	}

	FORCEINLINE UInt32 GetDescriptorSize() const
	{
		return DescriptorSize;
	}

private:
	void AllocateHeap();

private:
	TArray<DescriptorHeap> Heaps;
	std::string Name;

	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	UInt32 DescriptorSize = 0;
};

/*
* D3D12OnlineDescriptorHeap
*/

class D3D12OnlineDescriptorHeap : public D3D12RefCountedObject
{
public:
	D3D12OnlineDescriptorHeap(D3D12Device* InDevice, UInt32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);

	Bool Init();

	UInt32 AllocateHandles(UInt32 NumHandles);

	void Reset();

	FORCEINLINE void SetName(const std::string& Name)
	{
		Heap->SetName(Name);
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(UInt32 Index) const
	{
		return { Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(UInt32 Index) const
	{
		return { Heap->GetGPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
	}

	FORCEINLINE UInt32 GetDescriptorHandleIncrementSize() const
	{
		return Heap->GetDescriptorHandleIncrementSize();
	}
	
	FORCEINLINE ID3D12DescriptorHeap* GetNativeHeap() const
	{
		return Heap->GetHeap();
	}

	FORCEINLINE D3D12DescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}

private:
	TSharedRef<D3D12DescriptorHeap> Heap;
	TArray<TSharedRef<D3D12DescriptorHeap>> HeapPool;
	TArray<TSharedRef<D3D12DescriptorHeap>> DiscardedHeaps;
	
	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	UInt32 CurrentHandle		= 0;
	UInt32 DescriptorCount		= 0;
};