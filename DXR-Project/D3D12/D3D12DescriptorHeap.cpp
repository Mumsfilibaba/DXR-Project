#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12Views.h"

/*
* D3D12DescriptorHeap
*/

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device* InDevice, const D3D12_DESCRIPTOR_HEAP_DESC& InDesc)
	: D3D12RefCountedObject(InDevice)
	, Heap(nullptr)
	, CPUStart({ 0 })
	, GPUStart({ 0 })
	, DescriptorHandleIncrementSize(0)
	, Desc(InDesc)
{
}

Bool D3D12DescriptorHeap::Init()
{
	HRESULT Result = Device->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
	if (FAILED(Result))
	{
		LOG_ERROR("[D3D12DescriptorHeap]: FAILED to Create DescriptorHeap");
		Debug::DebugBreak();
		
		return false;
	}
	else
	{
		LOG_INFO("[D3D12DescriptorHeap]: Created DescriptorHeap");
	}


	CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
	GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();
	DescriptorHandleIncrementSize = Device->GetDescriptorHandleIncrementSize(Desc.Type);

	return true;
}

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
	if (Heap->Init())
	{
		if (!Name.empty())
		{
			std::string DbgName = Name + std::to_string(Heaps.Size());
			Heap->SetName(DbgName.c_str());
		}

		Heaps.EmplaceBack(Heap);
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

Bool D3D12OnlineDescriptorHeap::Init()
{
	Heap = Device->CreateDescriptorHeap(Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	if (Heap->Init())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void D3D12OnlineDescriptorHeap::Reset()
{
	if (!HeapPool.IsEmpty())
	{
		for (TSharedRef<D3D12DescriptorHeap>& CurrentHeap : DiscardedHeaps)
		{
			HeapPool.EmplaceBack(CurrentHeap);
		}

		DiscardedHeaps.Clear();
	}
	else
	{
		HeapPool.Swap(DiscardedHeaps);
	}

	CurrentHandle = 0;
}

UInt32 D3D12OnlineDescriptorHeap::AllocateHandles(UInt32 NumHandles)
{
	VALIDATE(NumHandles <= DescriptorCount);

	const UInt32 NewCurrentHandle = CurrentHandle + NumHandles;
	if (NewCurrentHandle >= DescriptorCount)
	{
		DiscardedHeaps.EmplaceBack(Heap);

		if (HeapPool.IsEmpty())
		{
			Heap = Device->CreateDescriptorHeap(Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
			if (!Heap->Init())
			{
				Debug::DebugBreak();
				return UInt32(-1);
			}
		}
		else
		{
			Heap = HeapPool.Back();
			HeapPool.PopBack();
		}

		CurrentHandle = 0;
	}

	const UInt32 Handle = CurrentHandle;
	CurrentHandle += NumHandles;
	return Handle;
}
