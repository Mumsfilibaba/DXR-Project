#include "Core/Threading/ScopedLock.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Query.h"

FD3D12QueryRHI::FD3D12QueryRHI(FD3D12Device* InDevice, EQueryType InQueryType)
    : FD3D12DeviceChild(InDevice)
    , FRHIQuery(InQueryType)
    , CurrentQuery()
    , SyncPoint()
    , QueryResult(static_cast<uint64*>(Memory::Malloc(GetQueryResultElementCount(InQueryType) * sizeof(uint64))))
{
    Memory::Memzero(QueryResult, GetQueryResultElementCount(InQueryType) * sizeof(uint64));
}

FD3D12QueryRHI::~FD3D12QueryRHI()
{
    Memory::Free(QueryResult);
    QueryResult = nullptr;
}

void FD3D12Query::CopyResult(void* Dst) const
{
    if (!QueryHeap || QueryIndex == D3D12_INVALID_QUERY_INDEX || !Dst)
    {
        return;
    }

    const uint64* MappedData = QueryHeap->GetReadbackData();
    if (!MappedData)
    {
        return;
    }

    const uint64 Stride = QueryHeap->GetQuerySize();
    Memory::Memcpy(Dst, reinterpret_cast<const uint8*>(MappedData) + QueryIndex * Stride, static_cast<size_t>(Stride));
}

FD3D12QueryHeap::FD3D12QueryHeap(FD3D12Device* InDevice, D3D12_QUERY_HEAP_TYPE InHeapType, int32 InNumQueries)
    : FD3D12DeviceChild(InDevice)
    , QueryHeapType(InHeapType)
    , QueryType(GetResolveQueryType(InHeapType))
    , NumQueries(InNumQueries)
    , ReadbackData(nullptr)
{
}

FD3D12QueryHeap::~FD3D12QueryHeap()
{
    if (ReadbackData && ReadbackResource.IsValid())
    {
        ReadbackResource->UnmapRange(0, nullptr);
        ReadbackData = nullptr;
    }

    if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
    {
        ResidencyManager->EndTrackingObject(&ResidencyHandle);
    }

#if D3D12_ENABLE_STATS
    if (QueryHeap.IsValid())
    {
        STAT_SUBTRACT(STAT_D3D12_QueryHeapCount, 1);
    }
#endif
}

bool FD3D12QueryHeap::Initialize()
{
    D3D12_QUERY_HEAP_DESC QueryHeapDesc = {};
    QueryHeapDesc.Type     = QueryHeapType;
    QueryHeapDesc.Count    = Math::Max<int32>(1, NumQueries);
    QueryHeapDesc.NodeMask = GetDevice()->GetNodeMask();

    TComPtr<ID3D12QueryHeap> NewQueryHeap;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&NewQueryHeap));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Query]: FAILED to create Query Heap");
        return false;
    }

    const uint64 ResultStride = GetQuerySize();

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = QueryHeapDesc.Count * ResultStride;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    if (!GetDevice()->CreateCommittedResource(Desc, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, ReadbackResource))
    {
        D3D12_ERROR_CRITICAL("Failed to create Query Readback resource");
        return false;
    }

    ReadbackResource->SetResourceStateMode(ED3D12ResourceStateMode::SingleState);
    ReadbackResource->SetDefaultState(D3D12_RESOURCE_STATE_COPY_DEST);

    QueryHeap = NewQueryHeap;

    ResidencyHandle.Initialize(QueryHeap.Get(), QueryHeapDesc.Count * ResultStride);
    if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
    {
        ResidencyManager->BeginTrackingObject(&ResidencyHandle);
    }

    void* Mapped = ReadbackResource->MapRange(0, nullptr);
    ReadbackData = reinterpret_cast<uint64*>(Mapped);
    if (!ReadbackData)
    {
        D3D12_ERROR_CRITICAL("[FD3D12QueryHeap]: FAILED to persistently map readback buffer");
        return false;
    }

#if D3D12_ENABLE_STATS
    STAT_ADD(STAT_D3D12_QueryHeapCount, 1);
#endif

    return true;
}

void FD3D12QueryHeap::SetDebugName(const String& InName)
{
    if (QueryHeap)
    {
        HRESULT Result = QueryHeap->SetPrivateData(WKPDID_D3DDebugObjectName, InName.Size(), *InName);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set queryheap name");
        }

        WString WideName = CharToWide(InName);
        Result = QueryHeap->SetName(*WideName);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set queryheap name");
        }
    }

    if (ReadbackResource.IsValid())
    {
        const String ResourceName = InName + "ReadBack Resource";
        ReadbackResource->SetDebugName(ResourceName);
    }
}

FD3D12QueryAllocator::FD3D12QueryAllocator(FD3D12Device* InDevice, D3D12_QUERY_HEAP_TYPE InHeapType)
    : FD3D12DeviceChild(InDevice)
    , HeapType(InHeapType)
{
}

FD3D12QueryAllocator::~FD3D12QueryAllocator()
{
}

bool FD3D12QueryAllocator::Allocate(FD3D12Query& OutQuery, uint64* ResultTarget, ED3D12QueryType InType)
{
    const bool bNeedNewHeap = Ranges.IsEmpty() || Ranges.LastElement().Count >= Ranges.LastElement().Heap->NumQueries;
    if (bNeedNewHeap)
    {
        FD3D12QueryHeap* Heap = GetDevice()->ObtainQueryHeap(HeapType);
        if (!Heap)
        {
            return false;
        }

        Ranges.Add(FD3D12QueryRange(Heap, 0, 0));
    }

    FD3D12QueryRange& Range = Ranges.LastElement();
    OutQuery = FD3D12Query(Range.Heap, Range.StartIndex + Range.Count, ResultTarget, InType);
    Range.Count++;
    return true;
}

void FD3D12QueryAllocator::Reset(TArray<FD3D12QueryRange>& OutRanges)
{
    for (FD3D12QueryRange& Range : Ranges)
    {
        OutRanges.Add(Move(Range));
    }

    Ranges.Clear();
}

FD3D12QueryHeapManager::FD3D12QueryHeapManager(FD3D12Device* InDevice, D3D12_QUERY_HEAP_TYPE InHeapType, int32 InQueriesPerHeap)
    : FD3D12DeviceChild(InDevice)
    , HeapType(InHeapType)
    , QueriesPerHeap(Math::Max<int32>(1, InQueriesPerHeap))
{
}

FD3D12QueryHeapManager::~FD3D12QueryHeapManager()
{
    TScopedLock Lock(HeapsCS);
    for (FD3D12QueryHeap* Heap : AllHeaps)
    {
        delete Heap;
    }
    AllHeaps.Clear();
}

FD3D12QueryHeap* FD3D12QueryHeapManager::ObtainHeap()
{
    TScopedLock Lock(HeapsCS);

    FD3D12QueryHeap* Heap = nullptr;
    if (AvailableHeaps.Dequeue(Heap))
    {
        return Heap;
    }

    Heap = new FD3D12QueryHeap(GetDevice(), HeapType, QueriesPerHeap);
    if (!Heap->Initialize())
    {
        delete Heap;
        return nullptr;
    }

    const String DebugName = String::CreateFormatted("QueryHeap [%d]", AllHeaps.Size());
    Heap->SetDebugName(DebugName);

    AllHeaps.Add(Heap);
    return Heap;
}

void FD3D12QueryHeapManager::RecycleHeap(FD3D12QueryHeap* Heap)
{
    TScopedLock Lock(HeapsCS);
    AvailableHeaps.Enqueue(Heap);
}
