#include "D3D12Descriptors.h"
#include "D3D12Device.h"
#include "D3D12ResourceViews.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"

TAutoConsoleVariable<int32> CVarNumOfflineDescriptors(
    "D3D12RHI.NumOfflineDescriptors",
    "The number of descriptors in each Offline DescriptorHeap",
    D3D12_MAX_OFFLINE_DESCRIPTOR_COUNT,
    EConsoleVariableFlags::Default);


FD3D12DescriptorHeap::FD3D12DescriptorHeap(FD3D12Device* InDevice, ID3D12DescriptorHeap* InHeap, D3D12_DESCRIPTOR_HEAP_TYPE InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags, uint32 InNumDescriptors)
    : FD3D12DeviceChild(InDevice)
    , Heap(MakeComPtr<ID3D12DescriptorHeap>(InHeap))
    , Type(InType)
    , Flags(InFlags)
    , NumDescriptors(InNumDescriptors)
{
    // Get the increment size
    HandleIncrementSize = GetDevice()->GetD3D12Device()->GetDescriptorHandleIncrementSize(Type);

    // Get the start-handles
    StartHandleCPU = Heap->GetCPUDescriptorHandleForHeapStart();
    if (Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        StartHandleGPU = Heap->GetGPUDescriptorHandleForHeapStart();
    }
}

FD3D12DescriptorHeap::FD3D12DescriptorHeap(FD3D12DescriptorHeap* InHeap, uint32 InHandleOffset, uint32 InNumDescriptors)
    : FD3D12DeviceChild(InHeap->GetDevice())
    , NumDescriptors(InNumDescriptors)
{
    CHECK(InHeap != nullptr);
    Heap  = InHeap->Heap;
    Type  = InHeap->Type;
    Flags = InHeap->Flags;
    HandleIncrementSize = InHeap->HandleIncrementSize;

    CHECK(InHandleOffset + NumDescriptors <= InHeap->NumDescriptors);
    StartHandleCPU = InHeap->GetCPUHandle(InHandleOffset);
    StartHandleGPU = InHeap->GetGPUHandle(InHandleOffset);
}


FD3D12OfflineDescriptorHeap::FD3D12OfflineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : FD3D12DeviceChild(InDevice)
    , Heaps()
    , Type(InType)
{
}

bool FD3D12OfflineDescriptorHeap::Initialize()
{
    DescriptorSize = GetDevice()->GetD3D12Device()->GetDescriptorHandleIncrementSize(Type);
    return AllocateHeap();
}

FD3D12OfflineDescriptor FD3D12OfflineDescriptorHeap::Allocate()
{
    TScopedLock Lock(HeapsCS);

    bool bFoundHeap = false;
    int32 HeapIndex = 0;
    for (FOfflineHeap& OfflineHeap : Heaps)
    {
        if (!OfflineHeap.FreeList.IsEmpty())
        {
            bFoundHeap = true;
            break;
        }
        else
        {
            HeapIndex++;
        }
    }

    if (!bFoundHeap)
    {
        if (!AllocateHeap())
        {
            return FD3D12OfflineDescriptor();
        }

        HeapIndex = static_cast<uint32>(Heaps.Size()) - 1;
    }

    FOfflineHeap& OfflineHeap = Heaps[HeapIndex];
    FD3D12DescriptorRange& Range = OfflineHeap.FreeList.FirstElement();

    FD3D12OfflineDescriptor Result(Range.Start, HeapIndex);
    Range.Start.ptr += DescriptorSize;

    if (!Range.IsValid())
    {
        OfflineHeap.FreeList.RemoveAt(0);
    }

    return Result;
}

void FD3D12OfflineDescriptorHeap::Free(FD3D12OfflineDescriptor& Descriptor)
{
    TScopedLock Lock(HeapsCS);

    CHECK(Heaps.IsValidIndex(Descriptor.HeapIndex));
    FOfflineHeap& Heap = Heaps[Descriptor.HeapIndex];

    bool bFoundRange = false;
    for (FD3D12DescriptorRange& Range : Heap.FreeList)
    {
        CHECK(Range.IsValid());

        if (Descriptor.Handle.ptr + DescriptorSize == Range.Start.ptr)
        {
            Range.Start = Descriptor.Handle;
            bFoundRange = true;
            break;
        }
        else if (Descriptor.Handle.ptr == Range.End.ptr)
        {
            Range.End.ptr += DescriptorSize;
            bFoundRange = true;
            break;
        }
    }

    if (!bFoundRange)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE End = { Descriptor.Handle.ptr + DescriptorSize };
        Heap.FreeList.Emplace(Descriptor.Handle, End);
    }

    Descriptor = FD3D12OfflineDescriptor();
}

bool FD3D12OfflineDescriptorHeap::AllocateHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Type           = Type;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NumDescriptors = FMath::Min(CVarNumOfflineDescriptors.GetValue(), D3D12_MAX_OFFLINE_DESCRIPTOR_COUNT);
    Desc.NodeMask       = GetDevice()->GetNodeMask();

    TComPtr<ID3D12DescriptorHeap> NewHeap;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&NewHeap));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12OfflineDescriptorHeap]: FAILED to Create DescriptorHeap");
        return false;
    }
    else
    {
        D3D12_INFO("[FD3D12OfflineDescriptorHeap]: Created DescriptorHeap");
    }

    FD3D12DescriptorHeapRef Heap = new FD3D12DescriptorHeap(GetDevice(), NewHeap.Get(), Desc.Type, Desc.Flags, Desc.NumDescriptors);
    Heaps.Emplace(Heap, Desc.NumDescriptors);
    return true;
}


FD3D12OnlineDescriptorHeap::FD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : FD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , DescriptorCount(0)
    , Type(InType)
{
}

bool FD3D12OnlineDescriptorHeap::Initialize(uint32 InDescriptorCount, uint32 InBlockSize)
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Type           = Type;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Desc.NumDescriptors = DescriptorCount = InDescriptorCount;
    Desc.NodeMask       = GetDevice()->GetNodeMask();

    TComPtr<ID3D12DescriptorHeap> NewHeap;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&NewHeap));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12OnlineDescriptorHeap]: FAILED to Create DescriptorHeap");
        return false;
    }
    else
    {
        D3D12_INFO("[FD3D12OnlineDescriptorHeap]: Created DescriptorHeap");
    }

    // Create Heap
    Heap = new FD3D12DescriptorHeap(GetDevice(), NewHeap.Get(), Desc.Type, Desc.Flags, Desc.NumDescriptors);

    // Divide the heap into blocks
    const uint32 NumBlocks = InDescriptorCount / InBlockSize;
    BlockSize = InBlockSize;

    uint32 HandleOffset = 0;
    for (uint32 Index = 0; Index < NumBlocks; Index++)
    {
        FD3D12OnlineDescriptorBlock* NewBlock = new FD3D12OnlineDescriptorBlock(HandleOffset, BlockSize);
        BlockQueue.Enqueue(NewBlock);
        HandleOffset += BlockSize;
    }

    return true;
}

FD3D12OnlineDescriptorBlock* FD3D12OnlineDescriptorHeap::AllocateBlock()
{
    TScopedLock Lock(BlockQueueCS);

    if (BlockQueue.IsEmpty())
    {
        return nullptr;
    }

    FD3D12OnlineDescriptorBlock* Block = nullptr;
    if (!BlockQueue.Dequeue(Block))
    {
        return nullptr;
    }

    return Block;
}

void FD3D12OnlineDescriptorHeap::RecycleBlock(FD3D12OnlineDescriptorBlock* InBlock)
{
    TScopedLock Lock(BlockQueueCS);
    BlockQueue.Enqueue(InBlock);
}

void FD3D12OnlineDescriptorHeap::FreeBlockDeferred(FD3D12OnlineDescriptorBlock* InBlock, uint64 FenceValue)
{
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(FenceValue, this, InBlock);
}
