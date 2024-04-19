#include "D3D12Device.h"
#include "D3D12CommandContext.h"
#include "D3D12Query.h"

FD3D12Query::FD3D12Query(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , FRHIQuery()
    , QueryHeap(nullptr)
    , WriteResource(nullptr)
    , ReadResources()
    , TimeQueries()
    , Frequency(0)
{
}

bool FD3D12Query::Initialize()
{
    ID3D12Device* D3D12Device = GetDevice()->GetD3D12Device();

    D3D12_QUERY_HEAP_DESC QueryHeapDesc;
    FMemory::Memzero(&QueryHeapDesc);

    QueryHeapDesc.Type     = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    QueryHeapDesc.Count    = D3D12_DEFAULT_QUERY_COUNT * 2;
    QueryHeapDesc.NodeMask = 0;

    TComPtr<ID3D12QueryHeap> Heap;
    HRESULT Result = D3D12Device->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&Heap));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Query]: FAILED to create Query Heap");
        return false;
    }

    D3D12_RESOURCE_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = D3D12_DEFAULT_QUERY_COUNT * sizeof(FTimingQuery);
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    FD3D12ResourceRef NewWriteResource = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!NewWriteResource->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
    {
        return false;
    }
    else
    {
        NewWriteResource->SetDebugName("Query Write Resource");
    }

    // Start with three
    for (uint32 Index = 0; Index < D3D12_NUM_BACK_BUFFERS; ++Index)
    {
        if (!AllocateReadResource())
        {
            return false;
        }
    }

    QueryHeap     = Heap;
    WriteResource = NewWriteResource;
    return true;
}

void FD3D12Query::GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const
{
    if (Index >= static_cast<uint32>(TimeQueries.Size()))
    {
        OutQuery.Begin = 0;
        OutQuery.End   = 0;
    }
    else
    {
        OutQuery = TimeQueries[Index];
    }
}

void FD3D12Query::BeginQuery(FD3D12CommandList& CommandList, uint32 Index)
{
    CHECK(Index < D3D12_DEFAULT_QUERY_COUNT);

    const uint32 FinalIndex = Index * 2;
    CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, FinalIndex);

    const uint32 QueryCount = Index + 1;
    if (QueryCount >= (uint32)TimeQueries.Size())
    {
        TimeQueries.Resize(QueryCount);
    }
}

void FD3D12Query::EndQuery(FD3D12CommandList& CommandList, uint32 Index)
{
    CHECK(Index < D3D12_DEFAULT_QUERY_COUNT);

    const uint32 FinalIndex = (Index * 2) + 1;
    CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, FinalIndex);
}

void FD3D12Query::ResolveQueries(class FD3D12CommandContext& CommandContext)
{
    ID3D12CommandQueue* D3D12Queue = GetDevice()->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct);
    CHECK(D3D12Queue != nullptr);

    uint32 ReadIndex = CommandContext.GetCurrentBatchIndex();
    if (ReadIndex >= (uint32)ReadResources.Size())
    {
        if (!AllocateReadResource())
        {
            DEBUG_BREAK();
            return;
        }
    }

    // NOTE: Read the current, the first frames the result will be zero, however this would be expected
    FD3D12Resource* CurrentReadResource = ReadResources[ReadIndex].Get();
    if (void* Data = CurrentReadResource->MapRange(0, nullptr))
    {
        const uint32 SizeInBytes = TimeQueries.SizeInBytes();
        FMemory::Memcpy(TimeQueries.Data(), Data, SizeInBytes);
        CurrentReadResource->UnmapRange(0, nullptr);
    }

    // Make use of RESOURCE_STATE promotion during the first call
    FD3D12CommandList& CommandList = CommandContext.GetCommandList();
    CommandList->ResolveQueryData(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, D3D12_DEFAULT_QUERY_COUNT, WriteResource->GetD3D12Resource(), 0);

    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource   = WriteResource->GetD3D12Resource();
    Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    CommandList->ResourceBarrier(1, &Barrier);

    CommandList->CopyResource(CurrentReadResource->GetD3D12Resource(), WriteResource->GetD3D12Resource());
    
    Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    CommandList->ResourceBarrier(1, &Barrier);

    HRESULT Result = D3D12Queue->GetTimestampFrequency(&Frequency);
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Query] FAILED to query ClockCalibration");
    }
}

bool FD3D12Query::AllocateReadResource()
{
    D3D12_RESOURCE_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = D3D12_DEFAULT_QUERY_COUNT * sizeof(FTimingQuery);
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    FD3D12ResourceRef ReadResource = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_READBACK);
    if (ReadResource->Initialize(D3D12_RESOURCE_STATE_COPY_DEST, nullptr))
    {
        ReadResource->SetDebugName("Query Readback Resource");
        ReadResources.Emplace(ReadResource);
    }
    else
    {
        return false;
    }

    return true;
}
