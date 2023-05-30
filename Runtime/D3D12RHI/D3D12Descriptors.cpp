#include "D3D12Descriptors.h"
#include "D3D12Device.h"
#include "D3D12ResourceViews.h"
#include "Core/Misc/FrameProfiler.h"

FD3D12DescriptorHeap::FD3D12DescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags)
    : FD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , CPUStart{0}
    , GPUStart{0}
    , DescriptorHandleIncrementSize(0)
    , Type(InType)
    , NumDescriptors(InNumDescriptors)
    , Flags(InFlags)
{
}

bool FD3D12DescriptorHeap::Initialize()
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Type           = Type;
    Desc.Flags          = Flags;
    Desc.NumDescriptors = NumDescriptors;

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12DescriptorHeap]: FAILED to Create DescriptorHeap");
        return false;
    }
    else
    {
        D3D12_INFO("[FD3D12DescriptorHeap]: Created DescriptorHeap");
    }

    CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
    if (Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();
    }

    DescriptorHandleIncrementSize = GetDevice()->GetD3D12Device()->GetDescriptorHandleIncrementSize(Desc.Type);
    return true;
}


FD3D12OfflineDescriptorHeap::FD3D12OfflineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : FD3D12DeviceChild(InDevice)
    , Heaps()
    , Name()
    , Type(InType)
{
}

bool FD3D12OfflineDescriptorHeap::Initialize()
{
    DescriptorSize = GetDevice()->GetD3D12Device()->GetDescriptorHandleIncrementSize(Type);
    return AllocateHeap();
}

D3D12_CPU_DESCRIPTOR_HANDLE FD3D12OfflineDescriptorHeap::Allocate(uint32& OutHeapIndex)
{
    TScopedLock Lock(CriticalSection);

    uint32 HeapIndex = 0;
    bool bFoundHeap = false;
    for (FDescriptorHeap& Heap : Heaps)
    {
        if (!Heap.FreeList.IsEmpty())
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
            return { 0 };
        }

        HeapIndex = static_cast<uint32>(Heaps.Size()) - 1;
    }

    FDescriptorHeap&  Heap  = Heaps[HeapIndex];
    FDescriptorRange& Range = Heap.FreeList.FirstElement();

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Range.Begin;
    Range.Begin.ptr += DescriptorSize;

    if (!Range.IsValid())
    {
        Heap.FreeList.RemoveAt(0);
    }

    OutHeapIndex = HeapIndex;
    return Handle;
}

void FD3D12OfflineDescriptorHeap::Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex)
{
    TScopedLock Lock(CriticalSection);
    CHECK(HeapIndex < (uint32)Heaps.Size());

    FDescriptorHeap& Heap = Heaps[HeapIndex];

    bool bFoundRange = false;
    for (FDescriptorRange& Range : Heap.FreeList)
    {
        CHECK(Range.IsValid());

        if (Handle.ptr + DescriptorSize == Range.Begin.ptr)
        {
            Range.Begin = Handle;
            bFoundRange = true;

            break;
        }
        else if (Handle.ptr == Range.End.ptr)
        {
            Range.End.ptr += DescriptorSize;
            bFoundRange = true;

            break;
        }
    }

    if (!bFoundRange)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE End = { Handle.ptr + DescriptorSize };
        Heap.FreeList.Emplace(Handle, End);
    }
}

void FD3D12OfflineDescriptorHeap::SetName(const FString& InName)
{
    Name = InName;

    uint32 HeapIndex = 0;
    for (FDescriptorHeap& Heap : Heaps)
    {
        FString DbgName = Name + "[" + TTypeToString<uint32>::ToString(HeapIndex) + "]";
        Heap.Heap->SetName(DbgName.GetCString());
    }
}

bool FD3D12OfflineDescriptorHeap::AllocateHeap()
{
    constexpr uint32 DescriptorCount = D3D12_MAX_OFFLINE_DESCRIPTOR_COUNT;

    TSharedRef<FD3D12DescriptorHeap> Heap = new FD3D12DescriptorHeap(GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    if (Heap->Initialize())
    {
        if (!Name.IsEmpty())
        {
            FString DbgName = Name + TTypeToString<int32>::ToString(Heaps.Size());
            Heap->SetName(DbgName.GetCString());
        }

        Heaps.Emplace(Heap);
        return true;
    }
    else
    {
        return false;
    }
}


FD3D12OnlineDescriptorHeap::FD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : FD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , DescriptorCount(InDescriptorCount)
    , Type(InType)
{
}

bool FD3D12OnlineDescriptorHeap::Initialize()
{
    Heap = new FD3D12DescriptorHeap(GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    if (Heap->Initialize())
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32 FD3D12OnlineDescriptorHeap::AllocateHandles(uint32 NumHandles)
{
    CHECK(NumHandles <= DescriptorCount);

    if (!HasSpace(NumHandles))
    {
        if (!AllocateFreshHeap())
        {
            return (uint32)-1;
        }
    }

    const uint32 Handle = CurrentHandle;
    CurrentHandle += NumHandles;
    return Handle;
}

bool FD3D12OnlineDescriptorHeap::AllocateFreshHeap()
{
    TRACE_FUNCTION_SCOPE();

    DiscardedHeaps.Emplace(Heap);

    if (HeapPool.IsEmpty())
    {
        Heap = new FD3D12DescriptorHeap(GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        if (!Heap->Initialize())
        {
            DEBUG_BREAK();
            return false;
        }
    }
    else
    {
        Heap = HeapPool.LastElement();
        HeapPool.Pop();
    }

    CurrentHandle = 0;
    return true;
}

bool FD3D12OnlineDescriptorHeap::HasSpace(uint32 NumHandles) const
{
    const uint32 NewCurrentHandle = CurrentHandle + NumHandles;
    return NewCurrentHandle < DescriptorCount;
}

void FD3D12OnlineDescriptorHeap::Reset()
{
    if (!HeapPool.IsEmpty())
    {
        for (TSharedRef<FD3D12DescriptorHeap>& CurrentHeap : DiscardedHeaps)
        {
            HeapPool.Emplace(CurrentHeap);
        }

        DiscardedHeaps.Clear();
    }
    else
    {
        ::Swap(HeapPool, DiscardedHeaps);
    }

    CurrentHandle = 0;
}

void FD3D12OnlineDescriptorHeap::SetNumPooledHeaps(uint32 NumHeaps)
{
    if (NumHeaps > static_cast<uint32>(HeapPool.Size()))
    {
        HeapPool.Resize(NumHeaps);
    }
}


FD3D12OnlineDescriptorManager::FD3D12OnlineDescriptorManager(FD3D12Device* InDevice, FD3D12DescriptorHeap* InDescriptorHeap, uint32 InDescriptorStartOffset, uint32 InDescriptorCount)
    : FD3D12DeviceChild(InDevice)
    , DescriptorHeap(MakeSharedRef<FD3D12DescriptorHeap>(InDescriptorHeap))
    , CPUStartHandle()
    , GPUStartHandle()
    , DescriptorStartOffset(InDescriptorStartOffset)
    , DescriptorCount(InDescriptorCount)
    , CurrentHandle(0)
{
    CHECK(DescriptorHeap != nullptr);
    DescriptorSize = DescriptorHeap->GetDescriptorHandleIncrementSize();
    CPUStartHandle = FD3D12CPUDescriptorHandle(DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), DescriptorStartOffset, DescriptorSize);
    GPUStartHandle = FD3D12GPUDescriptorHandle(DescriptorHeap->GetGPUDescriptorHandleForHeapStart(), DescriptorStartOffset, DescriptorSize);
}