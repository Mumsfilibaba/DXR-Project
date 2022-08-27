#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Fence.h"
#include "D3D12CommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12CommandQueue

class FD3D12CommandQueue 
    : public FD3D12DeviceChild
{
public:
    FD3D12CommandQueue(FD3D12Device* InDevice)
        : FD3D12DeviceChild(InDevice)
        , Queue(nullptr)
        , Desc()
    { }

    FORCEINLINE bool Initialize(D3D12_COMMAND_LIST_TYPE Type)
    {
        D3D12_COMMAND_QUEUE_DESC QueueDesc;
        FMemory::Memzero(&QueueDesc);

        QueueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        QueueDesc.NodeMask = 1;
        QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        QueueDesc.Type     = Type;

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
        if (SUCCEEDED(Result))
        {
            D3D12_INFO("[FD3D12CommandQueue]: Created CommandQueue");
            return true;
        }
        else
        {
            D3D12_ERROR("[FD3D12CommandQueue]: FAILED to create CommandQueue");
            return false;
        }
    }

    FORCEINLINE bool SignalFence(FD3D12Fence& Fence, uint64 FenceValue)
    {
        HRESULT Result = Queue->Signal(Fence.GetFence(), FenceValue);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12DeviceRemovedHandlerRHI(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE bool WaitForFence(FD3D12Fence& Fence, uint64 FenceValue)
    {
        HRESULT Result = Queue->Wait(Fence.GetFence(), FenceValue);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12DeviceRemovedHandlerRHI(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void ExecuteCommandList(FD3D12CommandList* CommandList)
    {
        ID3D12CommandList* CommandLists[] = { CommandList->GetCommandList() };
        Queue->ExecuteCommandLists(1, CommandLists);
    }

    FORCEINLINE void SetName(const FString& Name)
    {
        FStringWide WideDebugName = CharToWide(Name);
        Queue->SetName(WideDebugName.GetCString());
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