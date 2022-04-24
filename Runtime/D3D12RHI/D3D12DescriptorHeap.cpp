#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12RHIViews.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12DescriptorHeap

CD3D12DescriptorHeap::CD3D12DescriptorHeap(CD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags)
    : CD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , CPUStart({ 0 })
    , GPUStart({ 0 })
    , DescriptorHandleIncrementSize(0)
    , Type(InType)
    , NumDescriptors(InNumDescriptors)
    , Flags(InFlags)
{
}

bool CD3D12DescriptorHeap::Init()
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Type = Type;
    Desc.Flags = Flags;
    Desc.NumDescriptors = NumDescriptors;

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
    if (FAILED(Result))
    {
        D3D12_ERROR_ALWAYS("[D3D12DescriptorHeap]: FAILED to Create DescriptorHeap");
        return false;
    }
    else
    {
        LOG_INFO("[D3D12DescriptorHeap]: Created DescriptorHeap");
    }

    CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
    if (Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();
    }

    DescriptorHandleIncrementSize = GetDevice()->GetDescriptorHandleIncrementSize(Desc.Type);

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12OfflineDescriptorHeap

CD3D12OfflineDescriptorHeap::CD3D12OfflineDescriptorHeap(CD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : CD3D12DeviceChild(InDevice)
    , Heaps()
    , Name()
    , Type(InType)
{
}

bool CD3D12OfflineDescriptorHeap::Init()
{
    DescriptorSize = GetDevice()->GetDescriptorHandleIncrementSize(Type);
    return AllocateHeap();
}

D3D12_CPU_DESCRIPTOR_HANDLE CD3D12OfflineDescriptorHeap::Allocate(uint32& OutHeapIndex)
{
    uint32 HeapIndex = 0;
    bool bFoundHeap = false;
    for (SDescriptorHeap& Heap : Heaps)
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

    SDescriptorHeap& Heap = Heaps[HeapIndex];
    SDescriptorRange& Range = Heap.FreeList.FirstElement();

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Range.Begin;
    Range.Begin.ptr += DescriptorSize;

    if (!Range.IsValid())
    {
        Heap.FreeList.RemoveAt(0);
    }

    OutHeapIndex = HeapIndex;
    return Handle;
}

void CD3D12OfflineDescriptorHeap::Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex)
{
    Assert(HeapIndex < (uint32)Heaps.Size());

    SDescriptorHeap& Heap = Heaps[HeapIndex];

    bool bFoundRange = false;
    for (SDescriptorRange& Range : Heap.FreeList)
    {
        Assert(Range.IsValid());

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

void CD3D12OfflineDescriptorHeap::SetName(const String& InName)
{
    Name = InName;

    uint32 HeapIndex = 0;
    for (SDescriptorHeap& Heap : Heaps)
    {
        String DbgName = Name + "[" + ToString(HeapIndex) + "]";
        Heap.Heap->SetName(DbgName.CStr());
    }
}

bool CD3D12OfflineDescriptorHeap::AllocateHeap()
{
    constexpr uint32 DescriptorCount = D3D12_MAX_OFFLINE_DESCRIPTOR_COUNT;

    TSharedRef<CD3D12DescriptorHeap> Heap = dbg_new CD3D12DescriptorHeap(GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    if (Heap->Init())
    {
        if (!Name.IsEmpty())
        {
            String DbgName = Name + ToString(Heaps.Size());
            Heap->SetName(DbgName.CStr());
        }

        Heaps.Emplace(Heap);
        return true;
    }
    else
    {
        return false;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12OnlineDescriptorHeap 

CD3D12OnlineDescriptorHeap::CD3D12OnlineDescriptorHeap(CD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType)
    : CD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , DescriptorCount(InDescriptorCount)
    , Type(InType)
{
}

bool CD3D12OnlineDescriptorHeap::Init()
{
    Heap = dbg_new CD3D12DescriptorHeap(GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    if (Heap->Init())
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32 CD3D12OnlineDescriptorHeap::AllocateHandles(uint32 NumHandles)
{
    Assert(NumHandles <= DescriptorCount);

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

bool CD3D12OnlineDescriptorHeap::AllocateFreshHeap()
{
    TRACE_FUNCTION_SCOPE();

    DiscardedHeaps.Emplace(Heap);

    if (HeapPool.IsEmpty())
    {
        Heap = dbg_new CD3D12DescriptorHeap(GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        if (!Heap->Init())
        {
            CDebug::DebugBreak();
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

bool CD3D12OnlineDescriptorHeap::HasSpace(uint32 NumHandles) const
{
    const uint32 NewCurrentHandle = CurrentHandle + NumHandles;
    return NewCurrentHandle < DescriptorCount;
}

void CD3D12OnlineDescriptorHeap::Reset()
{
    if (!HeapPool.IsEmpty())
    {
        for (TSharedRef<CD3D12DescriptorHeap>& CurrentHeap : DiscardedHeaps)
        {
            HeapPool.Emplace(CurrentHeap);
        }

        DiscardedHeaps.Clear();
    }
    else
    {
        HeapPool.Swap(DiscardedHeaps);
    }

    CurrentHandle = 0;
}

void CD3D12OnlineDescriptorHeap::SetNumPooledHeaps(uint32 NumHeaps)
{
    if (NumHeaps > static_cast<uint32>(HeapPool.Size()))
    {
        HeapPool.Resize(NumHeaps);
    }
}
