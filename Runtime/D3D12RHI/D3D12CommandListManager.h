#pragma once
#include "D3D12CommandList.h"
#include "D3D12Fence.h"
#include "D3D12DeletionQueue.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Queue.h"

class FD3D12Device;
class FD3D12Query;

class FD3D12CommandListManager : public FD3D12DeviceChild
{
public:
    FD3D12CommandListManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandListManager();

    bool Initialize();
    void Release();
    void DestroyCommandLists();
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

private:
    ED3D12CommandQueueType      QueueType;
    D3D12_COMMAND_LIST_TYPE     CommandListType;
    FD3D12FenceManager          FenceManager;
    TComPtr<ID3D12CommandQueue> CommandQueue;

    TQueue<FD3D12CommandList*>  AvailableCommandLists;
    TArray<FD3D12CommandList*>  CommandLists;
    FCriticalSection            CommandListsCS;
};

struct FD3D12CommandPayload
{
    FD3D12CommandPayload(FD3D12Device* InDevice, FD3D12CommandListManager* InCommandListManager);
    ~FD3D12CommandPayload();

    void Finish();

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

    FD3D12CommandListManager*       CommandListManager;
    FD3D12Device*                   Device;
    FD3D12FenceSyncPoint            SyncPoint;
    TArray<FD3D12CommandAllocator*> CommandAllocators;
    TArray<FD3D12CommandList*>      CommandLists;
    TArray<FD3D12Query*>            Queries;
    TArray<FD3D12DeferredObject>    DeletionQueue;
};