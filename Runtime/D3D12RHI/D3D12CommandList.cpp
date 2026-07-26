#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12DeviceDebug.h"

FD3D12CommandAllocator::FD3D12CommandAllocator(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , Allocator(nullptr)
    , QueueType(InQueueType)
{
}

FD3D12CommandAllocator::~FD3D12CommandAllocator()
{
#if D3D12_ENABLE_STATS
    if (Allocator)
    {
        STAT_SUBTRACT(STAT_D3D12_CommandAllocatorCount, 1);
    }
#endif
}

bool FD3D12CommandAllocator::Initialize()
{
    const D3D12_COMMAND_LIST_TYPE Type = ToCommandListType(QueueType);

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
    if (SUCCEEDED(Result))
    {
        D3D12_INFO("[FD3D12CommandAllocator]: Created CommandAllocator");

    #if D3D12_ENABLE_STATS
        STAT_ADD(STAT_D3D12_CommandAllocatorCount, 1);
    #endif

        return true;
    }
    else
    {
        D3D12_ERROR_CRITICAL("[FD3D12CommandAllocator]: FAILED to create CommandAllocator");
        return false;
    }
}

bool FD3D12CommandAllocator::Reset()
{
    HRESULT Result = Allocator->Reset();
    D3D12RHICheckDeviceRemoved(GetDevice(), Result, "CommandAllocator::Reset");

    return SUCCEEDED(Result);
}

FD3D12CommandAllocatorManager::FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(QueueType))
    , AvailableAllocators()
    , CommandAllocators()
{
}

FD3D12CommandAllocatorManager::~FD3D12CommandAllocatorManager()
{
    for (FD3D12CommandAllocator* CommandAllocator : CommandAllocators)
    {
        delete CommandAllocator;
    }
}

FD3D12CommandAllocator* FD3D12CommandAllocatorManager::ObtainAllocator()
{
    TScopedLock Lock(CommandAllocatorsCS);

    FD3D12CommandAllocator* CommandAllocator;
    if (!AvailableAllocators.IsEmpty())
    {
        AvailableAllocators.Dequeue(CommandAllocator);
    }
    else
    {
        CommandAllocator = new FD3D12CommandAllocator(GetDevice(), QueueType);
        if (!CommandAllocator->Initialize())
        {
            DEBUG_BREAK();
            return nullptr;
        }

        CommandAllocators.Add(CommandAllocator);
    }

    return CommandAllocator;
}

void FD3D12CommandAllocatorManager::RecycleAllocator(FD3D12CommandAllocator* InAllocator)
{
    CHECK(InAllocator != nullptr);

    if (!InAllocator->Reset())
    {
        DEBUG_BREAK();
    }

    TScopedLock Lock(CommandAllocatorsCS);
    AvailableAllocators.Enqueue(InAllocator);
}

FD3D12CommandList::FD3D12CommandList(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , CmdList(nullptr)
    , CmdList1(nullptr)
    , CmdList2(nullptr)
    , CmdList3(nullptr)
    , CmdList4(nullptr)
    , CmdList5(nullptr)
    , CmdList6(nullptr)
    , NumCommands(0)
    , bIsReady(false)
{
}

FD3D12CommandList::~FD3D12CommandList()
{
#if D3D12_ENABLE_STATS
    if (CmdList)
    {
        STAT_SUBTRACT(STAT_D3D12_CommandListCount, 1);
    }
#endif
}

bool FD3D12CommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type, FD3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandList(1, Type, Allocator->GetD3D12Allocator(), InitalPipeline, IID_PPV_ARGS(&CmdList));
    if (SUCCEEDED(Result))
    {
        NumCommands = 0;
        CmdList->Close();

        LOG_INFO("[FD3D12CommandList]: Created CommandList");

    #if D3D12_ENABLE_STATS
        STAT_ADD(STAT_D3D12_CommandListCount, 1);
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_1
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList1>(&CmdList1)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList1");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_2
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList2>(&CmdList2)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList2");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_3
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList3>(&CmdList3)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList3");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_4
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList4>(&CmdList4)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList4");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_5
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList5>(&CmdList5)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList5");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_6
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList6>(&CmdList6)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList6");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_7
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList7>(&CmdList7)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList7");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_8
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList8>(&CmdList8)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList8");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_9
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList9>(&CmdList9)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList9");
        }
    #endif

    #if D3D12_USE_ID3D12COMMANDLIST_10
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList10>(&CmdList10)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList10");
        }
    #endif

        return true;
    }
    else
    {
        D3D12_ERROR_CRITICAL("[FD3D12CommandList]: FAILED to create CommandList");
        return false;
    }
}

bool FD3D12CommandList::Reset(FD3D12CommandAllocator* Allocator)
{
    ResidencySet.Reset();
    TimestampQueries.Clear();
    OcclusionQueries.Clear();
    PipelineStatsQueries.Clear();
    
    bIsReady       = true;
    BeginTimestamp = FD3D12Query();
    EndTimestamp   = FD3D12Query();

    HRESULT Result = CmdList->Reset(Allocator->GetD3D12Allocator(), nullptr);
    D3D12RHICheckDeviceRemoved(GetDevice(), Result, "CommandList::Reset");

    return SUCCEEDED(Result);
}

bool FD3D12CommandList::Close()
{
    bIsReady = false;

    HRESULT Result = CmdList->Close();
    D3D12RHICheckDeviceRemoved(GetDevice(), Result, "CommandList::Close");

    NumCommands = 0;
    return SUCCEEDED(Result);
}

void FD3D12CommandList::InsertBeginTimestamp(FD3D12QueryAllocator& Allocator)
{
    if (Allocator.Allocate(BeginTimestamp, nullptr, ED3D12QueryType::CommandListBegin))
    {
        EndQuery(BeginTimestamp);
    }
}

void FD3D12CommandList::InsertEndTimestamp(FD3D12QueryAllocator& Allocator)
{
    if (Allocator.Allocate(EndTimestamp, nullptr, ED3D12QueryType::CommandListEnd))
    {
        EndQuery(EndTimestamp);
    }
}

void FD3D12CommandList::BeginQuery(const FD3D12Query& Query)
{
    FD3D12QueryHeap* Heap = Query.QueryHeap;
    UpdateResidency(Heap->GetResidencyHandle());

    const D3D12_QUERY_TYPE D3DType = GetResolveQueryType(Heap->QueryHeapType);
    GetGraphicsCommandList()->BeginQuery(Heap->GetD3D12QueryHeap(), D3DType, Query.QueryIndex);

    if (Heap->QueryHeapType == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    {
        OcclusionQueries.Add(Query);
    }
    else
    {
        PipelineStatsQueries.Add(Query);
    }
}

void FD3D12CommandList::EndQuery(const FD3D12Query& Query)
{
    FD3D12QueryHeap* Heap = Query.QueryHeap;
    UpdateResidency(Heap->GetResidencyHandle());

    if (Heap->QueryHeapType == D3D12_QUERY_HEAP_TYPE_TIMESTAMP)
    {
        GetGraphicsCommandList()->EndQuery(Heap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, Query.QueryIndex);
        TimestampQueries.Add(Query);
    }
    else
    {
        const D3D12_QUERY_TYPE D3DType = GetResolveQueryType(Heap->QueryHeapType);
        GetGraphicsCommandList()->EndQuery(Heap->GetD3D12QueryHeap(), D3DType, Query.QueryIndex);
    }
}
