#pragma once
#include "D3D12CommandList.h"
#include "D3D12Fence.h"
#include "Core/Platform/CriticalSection.h"

class FD3D12Device;
class FD3D12CommandAllocator;

class FD3D12CommandListManager : public FD3D12DeviceChild
{
public:
    FD3D12CommandListManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);

    bool Initialize();

    FD3D12CommandListRef ObtainCommandList(FD3D12CommandAllocator& CommandAllocator, ID3D12PipelineState* InitialPipelineState);
    
    void ReleaseCommandList(FD3D12CommandListRef InCommandList);

    FD3D12FenceSyncPoint ExecuteCommandList(FD3D12CommandListRef InCommandList, bool bWaitForCompletion);

    FD3D12FenceManager& GetFenceManager() { return FenceManager; }

    FORCEINLINE ED3D12CommandQueueType  GetQueueType() const
    {
        return QueueType;
    }

    FORCEINLINE D3D12_COMMAND_LIST_TYPE GetCommandListType() const
    {
        return CommandListType;
    }
    
    FORCEINLINE ID3D12CommandQueue* GetD3D12CommandQueue() const
    {
        return CommandQueue.Get();
    }

private:
    ED3D12CommandQueueType       QueueType;
    D3D12_COMMAND_LIST_TYPE      CommandListType;

    FD3D12FenceManager           FenceManager;

    TComPtr<ID3D12CommandQueue>  CommandQueue;

    TArray<FD3D12CommandListRef> CommandLists;
    FCriticalSection             CommandListsCS;
};