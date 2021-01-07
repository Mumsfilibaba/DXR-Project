#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12Views.h"

/*
* D3D12OfflineDescriptorHeap
*/

D3D12OfflineDescriptorHeap::D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
	: D3D12RefCountedObject(InDevice)
	, Heaps()
	, Name()
	, Type(InType)
{
	// Get the size of this type of heap
	DescriptorSize = Device->GetDescriptorHandleIncrementSize(Type);

	// Allocate a new heap
	AllocateHeap();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12OfflineDescriptorHeap::Allocate(UInt32& OutHeapIndex)
{
	// Find a heap that is not empty
	UInt32 HeapIndex = 0;
	bool FoundHeap = false;
	for (DescriptorHeap& Heap : Heaps)
	{
		if (!Heap.FreeList.IsEmpty())
		{
			FoundHeap = true;
			break;
		}
		else
		{
			HeapIndex++;
		}
	}

	// If a heap is not found allocate a new one
	if (!FoundHeap)
	{
		AllocateHeap();
		HeapIndex = static_cast<UInt32>(Heaps.Size()) - 1;
	}

	// Get the heap and the first free range
	DescriptorHeap& Heap	= Heaps[HeapIndex];
	DescriptorRange& Range	= Heap.FreeList.Front();

	D3D12_CPU_DESCRIPTOR_HANDLE Handle = Range.Begin;
	Range.Begin.ptr += DescriptorSize;

	if (!Range.IsValid())
	{
		Heap.FreeList.Erase(Heap.FreeList.Begin());
	}

	OutHeapIndex = HeapIndex;
	return Handle;
}

void D3D12OfflineDescriptorHeap::Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, UInt32 HeapIndex)
{
	VALIDATE(HeapIndex < Heaps.Size());
	DescriptorHeap& Heap = Heaps[HeapIndex];

	// Find a suitable range
	bool FoundRange	= false;
	for (DescriptorRange& Range : Heap.FreeList)
	{
		VALIDATE(Range.IsValid());

		if (Handle.ptr + DescriptorSize == Range.Begin.ptr)
		{
			Range.Begin = Handle;
			FoundRange = true;

			break;
		}
		else if (Handle.ptr == Range.End.ptr)
		{
			Range.End.ptr += DescriptorSize;
			FoundRange = true;

			break;
		}
	}

	if (!FoundRange)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE End = { Handle.ptr + DescriptorSize };
		Heap.FreeList.EmplaceBack(Handle, End);
	}
}

void D3D12OfflineDescriptorHeap::SetName(const std::string& InName)
{
	Name = InName;

	UInt32 HeapIndex = 0;
	for (DescriptorHeap& Heap : Heaps)
	{
		std::string DbgName = Name + "[" + std::to_string(HeapIndex) + "]";
		Heap.Heap->SetName(DbgName.c_str());
	}
}

void D3D12OfflineDescriptorHeap::AllocateHeap()
{
	constexpr UInt32 DescriptorCount = 32;

	TSharedRef<D3D12DescriptorHeap> Heap = Device->CreateDescriptorHeap(Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	if (Heap)
	{
		LOG_INFO("[D3D12OfflineDescriptorHeap]: Created DescriptorHeap");

		if (!Name.empty())
		{
			std::string DbgName = Name + std::to_string(Heaps.Size());
			Heap->SetName(DbgName.c_str());
		}

		Heaps.EmplaceBack(Heap);
	}
	else
	{
		LOG_ERROR("[D3D12OfflineDescriptorHeap]: Failed to create DescriptorHeap");
	}
}

/*
* D3D12OnlineDescriptorHeap
*/

D3D12OnlineDescriptorHeap::D3D12OnlineDescriptorHeap(D3D12Device* InDevice, UInt32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType)
	: D3D12RefCountedObject(InDevice)
	, Heap(nullptr)
	, DescriptorCount(InDescriptorCount)
	, Type(InType)
{
}

bool D3D12OnlineDescriptorHeap::CreateHeap()
{
	Heap = Device->CreateDescriptorHeap(Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	if (Heap)
	{
		LOG_INFO("[D3D12OnlineDescriptorHeap]: Created DescriptorHeap");
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12OnlineDescriptorHeap]: FAILED to create DescriptorHeap");
		return false;
	}
}

UInt32 D3D12OnlineDescriptorHeap::AllocateHandles(UInt32 NumHandles)
{
	UInt32 Slot = CurrentSlot;
	CurrentSlot += NumHandles;

	VALIDATE(CurrentSlot < DescriptorCount);

	return Slot;
}