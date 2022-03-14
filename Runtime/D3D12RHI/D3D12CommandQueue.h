#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Fence.h"
#include "D3D12CommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12CommandQueue

class CD3D12CommandQueue : public CD3D12DeviceObject
{
public:

    FORCEINLINE CD3D12CommandQueue(CD3D12Device* InDevice)
        : CD3D12DeviceObject(InDevice)
        , Queue(nullptr)
        , Desc()
    {
    }

    FORCEINLINE bool Initialize(D3D12_COMMAND_LIST_TYPE Type)
    {
        D3D12_COMMAND_QUEUE_DESC QueueDesc;
        CMemory::Memzero(&QueueDesc);

        QueueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        QueueDesc.NodeMask = 1;
        QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        QueueDesc.Type     = Type;

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
        if (SUCCEEDED(Result))
        {
            D3D12_INFO("Created CommandQueue");
            return true;
        }
        else
        {
            D3D12_ERROR_ALWAYS("FAILED to create CommandQueue");
            return false;
        }
    }

    FORCEINLINE bool SignalFence(CD3D12Fence& Fence, uint64 FenceValue)
    {
        HRESULT Result = Queue->Signal(Fence.GetFence(), FenceValue);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12RHIDeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE bool WaitForFence(CD3D12Fence& Fence, uint64 FenceValue)
    {
        HRESULT Result = Queue->Wait(Fence.GetFence(), FenceValue);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12RHIDeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void ExecuteCommandList(CD3D12CommandList* CommandList)
    {
        ID3D12CommandList* CommandLists[] = { CommandList->GetCommandList() };
        Queue->ExecuteCommandLists(1, CommandLists);
    }

    FORCEINLINE void SetName(const String& Name)
    {
        Queue->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.CStr());
    }

    FORCEINLINE ID3D12CommandQueue* GetD3D12Queue() const
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