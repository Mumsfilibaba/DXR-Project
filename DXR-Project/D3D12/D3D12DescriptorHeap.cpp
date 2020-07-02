#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12Views.h"

/*
* D3D12OfflineDescriptorHeap
*/

D3D12OfflineDescriptorHeap::D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
	: D3D12DeviceChild(InDevice)
	, Heaps()
	, DebugName()
	, Type(InType)
{
	// Get the size of this type of heap
	DescriptorSize = Device->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	// Allocate a new heap
	AllocateHeap();
}

D3D12OfflineDescriptorHeap::~D3D12OfflineDescriptorHeap()
{
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12OfflineDescriptorHeap::Allocate(Uint32& OutHeapIndex)
{
	// Find a heap that is not empty
	Uint32	HeapIndex = 0;
	bool	FoundHeap = false;
	for (DescriptorHeap& Heap : Heaps)
	{
		if (!Heap.FreeList.empty())
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
		HeapIndex = static_cast<Uint32>(Heaps.size()) - 1;
	}

	// Get the heap and the first free range
	DescriptorHeap&	Heap	= Heaps[HeapIndex];
	FreeRange&		Range	= Heap.FreeList.front();

	D3D12_CPU_DESCRIPTOR_HANDLE Handle = Range.Begin;
	Range.Begin.ptr += DescriptorSize;

	if (!Range.IsValid())
	{
		Heap.FreeList.erase(Heap.FreeList.begin() + HeapIndex);
	}

	OutHeapIndex = HeapIndex;
	return Handle;
}

void D3D12OfflineDescriptorHeap::Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, Uint32 HeapIndex)
{
	VALIDATE(HeapIndex < Heaps.size());
	DescriptorHeap&	Heap = Heaps[HeapIndex];

	// Find a suitable range
	bool FoundRange	= false;
	for (FreeRange& Range : Heap.FreeList)
	{
		VALIDATE(Range.IsValid());

		if (Handle.ptr + DescriptorSize == Range.Begin.ptr)
		{
			Range.Begin = Handle;
			FoundRange = true;
		}
		else if (Handle.ptr == Range.End.ptr)
		{
			Range.End.ptr += DescriptorSize;
			FoundRange = true;
		}
	}

	if (!FoundRange)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE End = { Handle.ptr + DescriptorSize };
		Heap.FreeList.emplace_back(Handle, End);
	}
}

void D3D12OfflineDescriptorHeap::SetName(const std::string& InName)
{
	DebugName = ConvertToWide(InName);

	Uint32 HeapIndex = 0;
	for (DescriptorHeap& Heap : Heaps)
	{
		std::wstring Name = DebugName + L"[" + std::to_wstring(HeapIndex) + L"]";
		Heap.Heap->SetName(Name.c_str());
	}
}

void D3D12OfflineDescriptorHeap::AllocateHeap()
{
	constexpr Uint32 DescriptorCount = 32;

	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // These heaps are not visible to shaders
	HeapDesc.NodeMask		= 0;
	HeapDesc.NumDescriptors = DescriptorCount;
	HeapDesc.Type			= Type;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	HRESULT hResult = Device->GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12OfflineDescriptorHeap]: Created DescriptorHeap\n");

		if (!DebugName.empty())
		{
			std::wstring Name = DebugName + std::to_wstring(Heaps.size());
			Heap->SetName(Name.c_str());
		}

		FreeRange WholeRange;
		WholeRange.Begin	= Heap->GetCPUDescriptorHandleForHeapStart();
		WholeRange.End.ptr	= WholeRange.Begin.ptr + (DescriptorSize * DescriptorCount);
		Heaps.emplace_back(Heap, WholeRange);
	}
	else
	{
		::OutputDebugString("[D3D12OfflineDescriptorHeap]: Failed to create DescriptorHeap\n");
	}
}

/*
* D3D12OnlineDescriptorHeap
*/

D3D12OnlineDescriptorHeap::D3D12OnlineDescriptorHeap(D3D12Device* InDevice, Uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType)
	: D3D12DeviceChild(InDevice)
	, Heap(nullptr)
	, DescriptorCount(InDescriptorCount)
	, CPUHeapStart({ 0 })
	, GPUHeapStart({ 0 })
	, Type(InType)
{
	DescriptorSize = Device->GetDevice()->GetDescriptorHandleIncrementSize(Type);
}

D3D12OnlineDescriptorHeap::~D3D12OnlineDescriptorHeap()
{
}

bool D3D12OnlineDescriptorHeap::Initialize()
{
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.NodeMask		= 0;
	HeapDesc.NumDescriptors	= DescriptorCount;
	HeapDesc.Type			= Type;

	HRESULT hResult = Device->GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap));
	if (SUCCEEDED(hResult))
	{
		CPUHeapStart = Heap->GetCPUDescriptorHandleForHeapStart();
		GPUHeapStart = Heap->GetGPUDescriptorHandleForHeapStart();

		::OutputDebugString("[D3D12OnlineDescriptorHeap]: Created DescriptorHeap\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12OnlineDescriptorHeap]: Failed to create DescriptorHeap\n");
		return false;
	}
}

Uint32 D3D12OnlineDescriptorHeap::AllocateSlots(Uint32 NumSlots)
{
	Uint32 Slot = CurrentSlot;
	CurrentSlot += NumSlots;
	return Slot;
}

void D3D12OnlineDescriptorHeap::SetName(const std::string& InName)
{
	std::wstring WideName = ConvertToWide(InName);
	Heap->SetName(WideName.c_str());
}

/*
* D3D12DescriptorTable
*/

D3D12DescriptorTable::D3D12DescriptorTable(D3D12Device* InDevice, Uint32 InDescriptorCount)
	: Device(InDevice)
	, CPUTableStart({ 0 })
	, GPUTableStart({ 0 })
	, DescriptorCount(InDescriptorCount)
{
	OfflineHandles.resize(DescriptorCount);

	StartDescriptorSlot = Device->GetGlobalOnlineResourceHeap()->AllocateSlots(DescriptorCount);

	CPUTableStart = Device->GetGlobalOnlineResourceHeap()->GetCPUSlotAt(StartDescriptorSlot);
	GPUTableStart = Device->GetGlobalOnlineResourceHeap()->GetGPUSlotAt(StartDescriptorSlot);
}

D3D12DescriptorTable::~D3D12DescriptorTable()
{
}

void D3D12DescriptorTable::SetUnorderedAccessView(D3D12UnorderedAccessView* View, Uint32 SlotIndex)
{
	OfflineHandles[SlotIndex] = View->GetOfflineHandle();
	IsDirty = true;
}

void D3D12DescriptorTable::SetConstantBufferView(D3D12ConstantBufferView* View, Uint32 SlotIndex)
{
	OfflineHandles[SlotIndex] = View->GetOfflineHandle();
	IsDirty = true;
}

void D3D12DescriptorTable::SetShaderResourceView(D3D12ShaderResourceView* View, Uint32 SlotIndex)
{
	OfflineHandles[SlotIndex] = View->GetOfflineHandle();
	IsDirty = true;
}

void D3D12DescriptorTable::CopyDescriptors()
{
	if (IsDirty)
	{
		std::vector<Uint32> RangeSizes(OfflineHandles.size());
		for (Uint32& Size : RangeSizes)
		{
			Size = 1;
		}

		Device->GetDevice()->CopyDescriptors(1, &CPUTableStart, &DescriptorCount, DescriptorCount, OfflineHandles.data(), RangeSizes.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		IsDirty = false;

		OfflineHandles.clear();
		OfflineHandles.resize(DescriptorCount);
	}
}
