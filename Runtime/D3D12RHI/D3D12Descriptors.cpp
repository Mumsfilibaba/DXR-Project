#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12RHI.h"

static TAutoConsoleVariable<int32> CVarNumOfflineDescriptors(
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
    , Type(InType)
    , DescriptorSize(0)
    , NumTotalDescriptors(0)
    , Heaps()
    , HeapsCS()
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
    FD3D12DescriptorRange& Range = OfflineHeap.FreeList.First();

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
    Memory::Memzero(&Desc);

    Desc.Type           = Type;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NumDescriptors = Math::Min(CVarNumOfflineDescriptors.GetValue(), D3D12_MAX_OFFLINE_DESCRIPTOR_COUNT);
    Desc.NodeMask       = GetDevice()->GetNodeMask();

    TComPtr<ID3D12DescriptorHeap> NewHeap;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&NewHeap));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12OfflineDescriptorHeap]: FAILED to Create DescriptorHeap");
        return false;
    }
    else
    {
        NumTotalDescriptors += Desc.NumDescriptors;
        D3D12_INFO("[FD3D12OfflineDescriptorHeap]: Created DescriptorHeap. NumTotalDescriptors=%u", NumTotalDescriptors);
    }

    FD3D12DescriptorHeapRef Heap = new FD3D12DescriptorHeap(GetDevice(), NewHeap.Get(), Desc.Type, Desc.Flags, Desc.NumDescriptors);
    Heaps.Emplace(Heap, Desc.NumDescriptors);
    return true;
}


FD3D12OnlineDescriptorHeap::FD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : FD3D12DeviceChild(InDevice)
    , Type(InType)
    , DescriptorCount(0)
    , BlockSize(0)
    , BindlessReservedCount(0)
    , Heap(nullptr)
    , AvailableBlockQueue()
    , BlockQueue()
    , BlockQueueCS()
{
}

FD3D12OnlineDescriptorHeap::~FD3D12OnlineDescriptorHeap()
{
    for (FD3D12OnlineDescriptorBlock* Block : BlockQueue)
    {
        delete Block;
    }
}

bool FD3D12OnlineDescriptorHeap::Initialize(uint32 InDescriptorCount, uint32 InBlockSize, uint32 InBindlessReservedCount)
{
    CHECK(InBindlessReservedCount <= InDescriptorCount);

    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Type           = Type;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Desc.NumDescriptors = DescriptorCount = InDescriptorCount;
    Desc.NodeMask       = GetDevice()->GetNodeMask();

    TComPtr<ID3D12DescriptorHeap> NewHeap;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&NewHeap));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12OnlineDescriptorHeap]: FAILED to Create DescriptorHeap");
        return false;
    }
    else
    {
        D3D12_INFO("[FD3D12OnlineDescriptorHeap]: Created DescriptorHeap. NumDescriptors=%u BindlessReserved=%u",
            InDescriptorCount, InBindlessReservedCount);
    }

    Heap                  = new FD3D12DescriptorHeap(GetDevice(), NewHeap.Get(), Desc.Type, Desc.Flags, Desc.NumDescriptors);
    BlockSize             = InBlockSize;
    BindlessReservedCount = InBindlessReservedCount;

    const uint32 BlockRegionSize = (InDescriptorCount > InBindlessReservedCount) ? (InDescriptorCount - InBindlessReservedCount) : 0;
    const uint32 NumBlocks       = BlockRegionSize / InBlockSize;

    uint32 HandleOffset = InBindlessReservedCount;
    for (uint32 Index = 0; Index < NumBlocks; Index++)
    {
        FD3D12OnlineDescriptorBlock* NewBlock = new FD3D12OnlineDescriptorBlock(HandleOffset, BlockSize);
        AvailableBlockQueue.Enqueue(NewBlock);
        BlockQueue.Add(NewBlock);
        HandleOffset += BlockSize;
    }

    return true;
}

FD3D12OnlineDescriptorBlock* FD3D12OnlineDescriptorHeap::AllocateBlock()
{
    TScopedLock Lock(BlockQueueCS);

    if (AvailableBlockQueue.IsEmpty())
    {
        return nullptr;
    }

    FD3D12OnlineDescriptorBlock* Block = nullptr;
    if (!AvailableBlockQueue.Dequeue(Block))
    {
        return nullptr;
    }

    return Block;
}

void FD3D12OnlineDescriptorHeap::RecycleBlock(FD3D12OnlineDescriptorBlock* InBlock)
{
    TScopedLock Lock(BlockQueueCS);
    AvailableBlockQueue.Enqueue(InBlock);
}

FD3D12BindlessDescriptorHeap::FD3D12BindlessDescriptorHeap(FD3D12OnlineDescriptorHeap& InGlobalHeap, uint32 InCapacity)
    : FD3D12DeviceChild(InGlobalHeap.GetDevice())
    , AliasedHeap(nullptr)
    , HeapType(InGlobalHeap.GetHeap()->GetType())
    , Capacity(InCapacity)
    , NextFreshSlot(0)
    , FreeStack()
    , SlotSources()
    , AllocCS()
    , PendingWrites()
    , PendingWritesCS()
{
    CHECK(InGlobalHeap.GetHeap() != nullptr);
    CHECK(InCapacity <= InGlobalHeap.GetNumDescriptors());

    AliasedHeap = new FD3D12DescriptorHeap(InGlobalHeap.GetHeap(), 0, InCapacity);

    SlotSources.Resize(InCapacity);
    for (uint32 Index = 0; Index < InCapacity; ++Index)
    {
        SlotSources[Index] = D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }
}

FD3D12BindlessDescriptorHeap::~FD3D12BindlessDescriptorHeap()
{
}

FRHIDescriptorHandle FD3D12BindlessDescriptorHeap::Allocate(EDescriptorType InType)
{
    TScopedLock Lock(AllocCS);

    uint32 SlotIndex = 0;
    if (!FreeStack.IsEmpty())
    {
        SlotIndex = FreeStack.Last();
        FreeStack.Pop();
    }
    else
    {
        if (NextFreshSlot >= Capacity)
        {
            D3D12_ERROR("[FD3D12BindlessDescriptorHeap]: Out of bindless slots (Capacity=%u). Increase D3D12RHI.NumBindless*Descriptors.", Capacity);
            return FRHIDescriptorHandle();
        }

        SlotIndex = NextFreshSlot++;
    }

    return FRHIDescriptorHandle(InType, SlotIndex);
}

void FD3D12BindlessDescriptorHeap::Free(FRHIDescriptorHandle Handle)
{
    if (!Handle.IsValid())
    {
        return;
    }

    FD3D12DeviceRHI::DeferDeletion(this, Handle);
}

void FD3D12BindlessDescriptorHeap::RecycleSlot(FRHIDescriptorHandle Handle)
{
    if (!Handle.IsValid())
    {
        return;
    }

    const uint32 SlotIndex = Handle.Index;
    CHECK(SlotIndex < Capacity);

    {
        TScopedLock Lock(AllocCS);
        SlotSources[SlotIndex] = D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
        FreeStack.Add(SlotIndex);
    }
}

void FD3D12BindlessDescriptorHeap::EnqueueWrite(FRHIDescriptorHandle Handle, D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle)
{
    if (!Handle.IsValid() || OfflineHandle.ptr == 0)
    {
        return;
    }

    const uint32 SlotIndex = Handle.Index;
    CHECK(SlotIndex < Capacity);

    TScopedLock Lock(PendingWritesCS);

    SlotSources[SlotIndex] = OfflineHandle;

    FD3D12PendingBindlessWrite& Write = PendingWrites.Emplace();
    Write.DestSlot  = SlotIndex;
    Write.SrcHandle = OfflineHandle;
}

void FD3D12BindlessDescriptorHeap::WriteSlotImmediate(FRHIDescriptorHandle Handle, D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle)
{
    if (!Handle.IsValid() || OfflineHandle.ptr == 0)
    {
        return;
    }

    const uint32 SlotIndex = Handle.Index;
    CHECK(SlotIndex < Capacity);

    TScopedLock Lock(PendingWritesCS);

    SlotSources[SlotIndex] = OfflineHandle;

    FD3D12DescriptorHeap* DestHeap = AliasedHeap.Get();
    CHECK(DestHeap != nullptr);

    GetDevice()->GetD3D12Device()->CopyDescriptorsSimple(
        1,
        DestHeap->GetCPUHandle(static_cast<int32>(SlotIndex)),
        OfflineHandle,
        HeapType);
}

void FD3D12BindlessDescriptorHeap::Flush()
{
    TArray<FD3D12PendingBindlessWrite> LocalWrites;
    {
        TScopedLock Lock(PendingWritesCS);
        if (PendingWrites.IsEmpty())
        {
            return;
        }

        LocalWrites = Move(PendingWrites);
        PendingWrites.Clear();
    }

    const uint32 NumWrites = static_cast<uint32>(LocalWrites.Size());
    if (NumWrites == 0)
    {
        return;
    }

    TArray<D3D12_CPU_DESCRIPTOR_HANDLE> SrcStarts;
    TArray<D3D12_CPU_DESCRIPTOR_HANDLE> DestStarts;

    SrcStarts.Resize(NumWrites);
    DestStarts.Resize(NumWrites);

    FD3D12DescriptorHeap* DestHeap = AliasedHeap.Get();
    CHECK(DestHeap != nullptr);

    for (uint32 Index = 0; Index < NumWrites; ++Index)
    {
        const FD3D12PendingBindlessWrite& Write = LocalWrites[Index];
        SrcStarts[Index]  = Write.SrcHandle;
        DestStarts[Index] = DestHeap->GetCPUHandle(static_cast<int32>(Write.DestSlot));
    }

    GetDevice()->GetD3D12Device()->CopyDescriptors(
        NumWrites,
        DestStarts.Data(),
        nullptr,
        NumWrites,
        SrcStarts.Data(),
        nullptr,
        HeapType);
}
