#pragma once
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/Map.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12DeletionQueue.h"
#include "D3D12RHI/D3D12ResourceState.h"
#include "D3D12RHI/D3D12Query.h"

class FD3D12Device;

enum class ED3D12CommandsFlags : uint32
{
    None           = 0,
    ResolveQueries = 1 << 0,
};

ENUM_CLASS_OPERATORS(ED3D12CommandsFlags);

struct FD3D12Commands
{
    FD3D12Commands(FD3D12Device* InDevice, FD3D12Queue* InQueue);
    ~FD3D12Commands() = default;

    void PreExecute();
    void Execute();
    void PostExecute();

    void AddCommandList(FD3D12CommandList* InCommandList)
    {
        CommandLists.Add(InCommandList);
    }

    void AddCommandAllocator(FD3D12CommandAllocator* InCommandAllocator)
    {
        CommandAllocators.Add(InCommandAllocator);
    }

    bool IsEmpty() const
    {
        return CommandLists.IsEmpty();
    }

    FD3D12Queue* const                         Queue;
    FD3D12Device* const                        Device;
    ED3D12CommandsFlags                        Flags;
    FD3D12FenceSyncPoint                       SyncPoint;
    TArray<FD3D12CommandAllocator*>            CommandAllocators;
    TArray<FD3D12CommandList*>                 CommandLists;
    TArray<FD3D12QueryRange>                   QueryRanges;
    TArray<FD3D12Query>                        TimestampQueries;
    TArray<FD3D12Query>                        OcclusionQueries;
    TArray<FD3D12Query>                        PipelineStatsQueries;
    TArray<struct FD3D12QueryRHI*>             PendingQueries;
    TArray<FD3D12DeferredObject>               DeferredObjects;
    TArray<FD3D12PendingBarrier>               PendingBarriers;
    TMap<FD3D12Resource*, FD3D12ResourceState> PendingResourceStates;
};

class FD3D12Queue : public FD3D12DeviceChild, FNonCopyable
{
public:
    FD3D12Queue(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12Queue();

    bool Initialize();

    FD3D12CommandList* ObtainCommandList(FD3D12CommandAllocator* CommandAllocator, ID3D12PipelineState* InitialPipelineState);
    void RecycleCommandList(FD3D12CommandList* InCommandList);
    
    FD3D12FenceSyncPoint ExecuteCommandList(FD3D12CommandList* InCommandList, bool bWaitForCompletion);
    FD3D12FenceSyncPoint ExecuteCommandLists(FD3D12CommandList* const* InCommandLists, uint32 NumCommandLists, bool bWaitForCompletion);

    void SubmitCommands(FD3D12Commands* Commands);
    void ProcessCommandQueue();

    FD3D12Fence& GetSubmissionFence()
    {
        return *SubmissionFence;
    }

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

    D3D12_COMMAND_LIST_TYPE GetCommandListType() const
    {
        return CommandListType;
    }
    
    ID3D12CommandQueue* GetD3D12CommandQueue() const
    {
        return CommandQueue.Get();
    }

    UINT64 GetFrequency() const
    {
        return Frequency;
    }

private:
    typedef TQueue<FD3D12Commands*, EQueueType::MPSC> FCommandsQueue;

    ED3D12CommandQueueType const QueueType;
    D3D12_COMMAND_LIST_TYPE      CommandListType;
    UINT64                       Frequency;
    FD3D12FenceRef               SubmissionFence;
    TComPtr<ID3D12CommandQueue>  CommandQueue;
    TQueue<FD3D12CommandList*>   AvailableCommandLists;
    TArray<FD3D12CommandList*>   CommandLists;
    FCriticalSection             CommandListsCS;
    FCommandsQueue               PendingSubmissions;
    FCriticalSection             SubmissionCS;
    TArray<FD3D12QueryRange>     PendingQueryRanges;
    TArray<FD3D12Query>          PendingTimestampQueries;
    TArray<FD3D12Query>          PendingOcclusionQueries;
    TArray<FD3D12Query>          PendingPipelineStatsQueries;
    TArray<FD3D12QueryRHI*>      PendingQueryRHIs;
};
