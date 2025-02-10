#pragma once
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Queue.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12DeletionQueue.h"

class FD3D12Device;
class FD3D12QueryHeap;

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

    FD3D12FenceManager& GetFenceManager() 
    {
        return FenceManager;
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
    ED3D12CommandQueueType const QueueType;
    D3D12_COMMAND_LIST_TYPE      CommandListType;
    UINT64                       Frequency;
    FD3D12FenceManager           FenceManager;
    TComPtr<ID3D12CommandQueue>  CommandQueue;
    TQueue<FD3D12CommandList*>   AvailableCommandLists;
    TArray<FD3D12CommandList*>   CommandLists;
    FCriticalSection             CommandListsCS;
};

struct FD3D12CommandPayload
{
    FD3D12CommandPayload(FD3D12Device* InDevice, FD3D12Queue* InQueue);
    ~FD3D12CommandPayload() = default;

    void Finish();

    void AddQueryHeap(FD3D12QueryHeap* InQueryHeap)
    {
        QueryHeaps.Add(InQueryHeap);
    }

    void AddCommandAllocator(FD3D12CommandAllocator* InCommandAllocator)
    {
        CommandAllocators.Add(InCommandAllocator);
    }

    void AddCommandList(FD3D12CommandList* InCommandList)
    {
        CommandLists.Add(InCommandList);
    }

    bool IsEmpty() const
    {
        return CommandLists.IsEmpty();
    }

    FD3D12Queue* const              Queue;
    FD3D12Device* const             Device;
    FD3D12FenceSyncPoint            SyncPoint;
    TArray<FD3D12CommandAllocator*> CommandAllocators;
    TArray<FD3D12CommandList*>      CommandLists;
    TArray<FD3D12QueryHeap*>        QueryHeaps;
    TArray<FD3D12DeferredObject>    DeletionQueue;
};