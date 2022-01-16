#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Fence.h"
#include "D3D12CommandList.h"

class CD3D12CommandQueue : public CD3D12DeviceChild
{
public:

    FORCEINLINE CD3D12CommandQueue(CD3D12Device* InDevice)
        : CD3D12DeviceChild(InDevice)
        , Queue(nullptr)
        , Desc()
    {
    }

    FORCEINLINE bool Init(D3D12_COMMAND_LIST_TYPE Type)
    {
        D3D12_COMMAND_QUEUE_DESC QueueDesc;
        CMemory::Memzero(&QueueDesc);

        QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        QueueDesc.NodeMask = 1;
        QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        QueueDesc.Type = Type;

        HRESULT Result = GetDevice()->GetDevice()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
        if (SUCCEEDED(Result))
        {
            LOG_INFO("[CD3D12CommandQueue]: Created CommandQueue");
            return true;
        }
        else
        {
            LOG_ERROR("[CD3D12CommandQueue]: FAILED to create CommandQueue");
            return false;
        }
    }

    FORCEINLINE bool SignalFence(CD3D12Fence& Fence, uint64 FenceValue)
    {
        HRESULT Result = Queue->Signal(Fence.GetFence(), FenceValue);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            RHID3D12DeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE bool WaitForFence(CD3D12Fence& Fence, uint64 FenceValue)
    {
        HRESULT Result = Queue->Wait(Fence.GetFence(), FenceValue);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            RHID3D12DeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void ExecuteCommandList(CD3D12CommandList* CommandList)
    {
        ID3D12CommandList* CommandLists[] = { CommandList->GetCommandList() };
        Queue->ExecuteCommandLists(1, CommandLists);
    }

    FORCEINLINE void SetName(const CString& Name)
    {
        WString WideDebugName = CharToWide(Name);
        Queue->SetName(WideDebugName.CStr());
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