#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Fence.h"
#include "D3D12CommandList.h"

class D3D12CommandQueueHandle : public D3D12DeviceChild
{
public:
    D3D12CommandQueueHandle(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , Queue(nullptr)
        , Desc()
    {
    }
    
    FORCEINLINE Bool Init(D3D12_COMMAND_LIST_TYPE Type)
    {
        D3D12_COMMAND_QUEUE_DESC QueueDesc;
        Memory::Memzero(&QueueDesc);

        QueueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        QueueDesc.NodeMask = 1;
        QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        QueueDesc.Type     = Type;

        HRESULT Result = Device->GetDevice()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
        if (SUCCEEDED(Result))
        {
            LOG_INFO("[D3D12CommandQueueHandle]: Created CommandQueue");
            return true;
        }
        else
        {
            LOG_ERROR("[D3D12CommandQueueHandle]: FAILED to create CommandQueue");
            return false;
        }
    }

    FORCEINLINE Bool SignalFence(D3D12FenceHandle& Fence, UInt64 FenceValue)
    {
        return SUCCEEDED(Queue->Signal(Fence.GetFence(), FenceValue));
    }

    FORCEINLINE Bool WaitForFence(D3D12FenceHandle& Fence, UInt64 FenceValue)
    {
        return SUCCEEDED(Queue->Wait(Fence.GetFence(), FenceValue));
    }

    FORCEINLINE void ExecuteCommandList(D3D12CommandListHandle* CommandList)
    {
        ID3D12CommandList* CommandLists[] = { CommandList->GetCommandList() };
        Queue->ExecuteCommandLists(1, CommandLists);
    }

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideDebugName = ConvertToWide(Name);
        Queue->SetName(WideDebugName.c_str());
    }

    FORCEINLINE ID3D12CommandQueue* GetQueue() const
    {
        return Queue.Get();
    }

    FORCEINLINE const D3D12_COMMAND_QUEUE_DESC& GetDesc() const
    {
        return Desc;
    }

    FORCEINLINE D3D12_COMMAND_LIST_TYPE GetType() const
    {
        return Desc.Type;
    }

private:
    TComPtr<ID3D12CommandQueue> Queue;
    D3D12_COMMAND_QUEUE_DESC    Desc;
};