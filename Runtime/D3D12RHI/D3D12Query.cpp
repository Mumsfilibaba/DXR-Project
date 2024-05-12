#include "D3D12Device.h"
#include "D3D12CommandContext.h"
#include "D3D12Query.h"
#include "D3D12Queue.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<int32> CVarNumTimestampQueriesPerHeap(
    "D3D12RHI.NumTimestampQueriesPerHeap",
    "The number of Timestamp Queries in each QueryHeap",
    D3D12_DEFAULT_QUERY_COUNT,
    EConsoleVariableFlags::Default);

static uint64 ToNanoseconds(uint64 Timestamp, uint64 Frequency)
{
    const double NanosecondScale = 1000'000'000.0 / static_cast<double>(Frequency);
    return static_cast<uint64>(static_cast<double>(Timestamp) * NanosecondScale);
}

FD3D12Query::FD3D12Query(FD3D12Device* InDevice, EQueryType InQueryType)
    : FD3D12DeviceChild(InDevice)
    , FRHIQuery(InQueryType)
    , QueryAllocation()
    , Result(0)
{
}

FD3D12QueryHeap::FD3D12QueryHeap(FD3D12Device* InDevice, FD3D12QueryHeapManager* InQueryHeapManager)
    : FD3D12DeviceChild(InDevice)
    , ReadResource(nullptr)
    , QueryHeap(nullptr)
    , QueryAllocations()
    , QueryHeapType()
    , CurrentQueryIndex(0)
    , NumQueries(0)
    , QueryHeapManager(InQueryHeapManager)
{
}

bool FD3D12QueryHeap::Initialize(D3D12_QUERY_HEAP_TYPE InQueryHeapType)
{
    D3D12_QUERY_HEAP_DESC QueryHeapDesc;
    FMemory::Memzero(&QueryHeapDesc);

    QueryHeapDesc.Type     = InQueryHeapType;
    QueryHeapDesc.Count    = CVarNumTimestampQueriesPerHeap.GetValue();
    QueryHeapDesc.NodeMask = GetDevice()->GetNodeMask();

    TComPtr<ID3D12QueryHeap> NewQueryHeap;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&NewQueryHeap));
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
    Desc.Width              = QueryHeapDesc.Count * sizeof(uint64); 
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    FD3D12ResourceRef NewResource = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_READBACK);
    if (!NewResource->Initialize(D3D12_RESOURCE_STATE_COPY_DEST, nullptr))
    {
        D3D12_ERROR("Failed to create Query Readback resource");
        return false;
    }

    QueryHeap = NewQueryHeap;
    ReadResource = NewResource;
    QueryHeapType = InQueryHeapType;
    NumQueries = QueryHeapDesc.Count;
    QueryAllocations.Resize(NumQueries);
    return true;
}

FD3D12QueryAllocation FD3D12QueryHeap::AllocateQueries(uint64* Results)
{
    int32 Index = CurrentQueryIndex++;
    if (Index >= NumQueries)
    {
        return FD3D12QueryAllocation();
    }

    QueryAllocations[Index] = FD3D12QueryAllocation(this, Index, Results);
    return QueryAllocations[Index];
}

void FD3D12QueryHeap::ResolveQueries(FD3D12CommandList& CommandList)
{
    D3D12_QUERY_TYPE QueryType;
    if (QueryHeapType == D3D12_QUERY_HEAP_TYPE_TIMESTAMP)
    {
        QueryType = D3D12_QUERY_TYPE_TIMESTAMP;
    }
    else if (QueryHeapType == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    {
        QueryType = D3D12_QUERY_TYPE_OCCLUSION;
    }
    else
    {
        DEBUG_BREAK();
    }

    const uint32 NumUsedQueries = FMath::Min<int32>(CurrentQueryIndex, NumQueries);
    CommandList->ResolveQueryData(QueryHeap.Get(), QueryType, 0, NumUsedQueries, ReadResource->GetD3D12Resource(), 0);
}

void FD3D12QueryHeap::ReadBackResults(FD3D12Queue& Queue)
{
    void* Data = ReadResource->MapRange(0, nullptr);
    if (!Data)
    {
        D3D12_ERROR("Failed to read query results");
        return;
    }

    const int32 NumUsedQueries = FMath::Min<int32>(CurrentQueryIndex, NumQueries);
    if (QueryHeapType == D3D12_QUERY_HEAP_TYPE_TIMESTAMP)
    {
        const uint64 Frequency = Queue.GetFrequency();
        const uint64* TimestampResults = reinterpret_cast<uint64*>(Data);
        for (int32 Index = 0; Index < NumUsedQueries; Index++)
        {
            uint64* Result = QueryAllocations[Index].Results;
            *Result = ToNanoseconds(TimestampResults[Index], Frequency);
        }
    }
    else if (QueryHeapType == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    {
        const uint64* OcclusionResults = reinterpret_cast<uint64*>(Data);
        for (int32 Index = 0; Index < NumUsedQueries; Index++)
        {
            uint64* Result = QueryAllocations[Index].Results;
            *Result = OcclusionResults[Index];
        }
    }

    ReadResource->UnmapRange(0, nullptr);

    // Reset this heap for future use
    CurrentQueryIndex = 0;
}

void FD3D12QueryHeap::SetDebugName(const FString& InName)
{
    if (QueryHeap)
    {
        HRESULT Result = QueryHeap->SetPrivateData(WKPDID_D3DDebugObjectName, InName.Size(), InName.GetCString());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set queryheap name");
        }

        // Calling SetName as well since NVIDIA NSight does not recognize the name otherwise
        FStringWide WideName = CharToWide(InName);
        Result = QueryHeap->SetName(WideName.GetCString());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set queryheap name");
        }
    }

    const FString ResourceName = InName + "ReadBack Resource";
    ReadResource->SetDebugName(ResourceName);
}

FD3D12QueryAllocator::FD3D12QueryAllocator(FD3D12Device* InDevice, FD3D12CommandContext& InContext, EQueryType InQueryType)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , QueryHeap(nullptr)
    , QueryType(InQueryType)
{
    QueryHeapManager = InDevice->GetQueryHeapManager(QueryType);
    CHECK(QueryHeapManager != nullptr);
}

FD3D12QueryAllocator::~FD3D12QueryAllocator()
{
    // NOTE: The QueryHeap should have been released when the context flushed it's CommandBuffer
    CHECK(QueryHeap == nullptr);
}

FD3D12QueryAllocation FD3D12QueryAllocator::Allocate(uint64* InResults)
{
    if (!QueryHeap)
    {
        QueryHeap = QueryHeapManager->ObtainQueryHeap();
    }

    FD3D12QueryAllocation QueryAllocation = QueryHeap->AllocateQueries(InResults);
    if (!QueryAllocation.IsValid())
    {
        Context.GetCommandPayload().AddQueryHeap(QueryHeap);
        QueryHeap = QueryHeapManager->ObtainQueryHeap();
        QueryAllocation = QueryHeap->AllocateQueries(InResults);
    }

    return QueryAllocation;
}

void FD3D12QueryAllocator::PrepareForNewCommanBuffer()
{
    if (QueryHeap)
    {
        Context.GetCommandPayload().AddQueryHeap(QueryHeap);
        QueryHeap = nullptr;
    }
}

FD3D12QueryHeapManager::FD3D12QueryHeapManager(FD3D12Device* InDevice, EQueryType InQueryType)
    : FD3D12DeviceChild(InDevice)
    , QueryType(InQueryType)
    , AvailableQueryHeaps()
    , QueryHeaps()
    , QueryHeapsCS()
{
}

FD3D12QueryHeapManager::~FD3D12QueryHeapManager()
{
    TScopedLock Lock(QueryHeapsCS);

    for (FD3D12QueryHeap* QueryHeap : QueryHeaps)
    {
        delete QueryHeap;
    }

    QueryHeaps.Clear();
    AvailableQueryHeaps.Clear();
}

FD3D12QueryHeap* FD3D12QueryHeapManager::ObtainQueryHeap()
{
    TScopedLock Lock(QueryHeapsCS);

    FD3D12QueryHeap* QueryHeap = nullptr;
    if (AvailableQueryHeaps.Dequeue(QueryHeap))
    {
        return QueryHeap;
    }

    const D3D12_QUERY_HEAP_TYPE QueryHeapType = ToQueryHeapType(QueryType);
    QueryHeap = new FD3D12QueryHeap(GetDevice(), this);
    if (!QueryHeap->Initialize(QueryHeapType))
    {
        DEBUG_BREAK();
        delete QueryHeap;
        return nullptr;
    }

    const FString DebugName = FString::CreateFormatted("QueryHeap [%s %d]", ToString(QueryType), QueryHeaps.Size());
    QueryHeap->SetDebugName(DebugName);

    QueryHeaps.Add(QueryHeap);
    return QueryHeap;
}

void FD3D12QueryHeapManager::RecycleQueryHeap(FD3D12QueryHeap* InQueryHeap)
{
    TScopedLock Lock(QueryHeapsCS);
    AvailableQueryHeaps.Enqueue(InQueryHeap);
}