#pragma once
#include "D3D12RefCountedObject.h"
#include "D3D12Fence.h"
#include "D3D12CommandList.h"

class D3D12CommandQueue : public D3D12RefCountedObject
{
public:
    D3D12CommandQueue(D3D12Device* InDevice, ID3D12CommandQueue* InQueue)
        : D3D12RefCountedObject(InDevice)
        , Queue(InQueue)
        , Desc()
    {
        VALIDATE(Queue != nullptr);
        Desc = Queue->GetDesc();
    }

    FORCEINLINE bool SignalFence(D3D12Fence* Fence, UInt64 FenceValue)
    {
        return SUCCEEDED(Queue->Signal(Fence->GetFence(), FenceValue));
    }

    FORCEINLINE bool WaitForFence(D3D12Fence* Fence, UInt64 FenceValue)
    {
        return SUCCEEDED(Queue->Wait(Fence->GetFence(), FenceValue));
    }

    FORCEINLINE void ExecuteCommandList(D3D12CommandList* CommandList)
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
    D3D12_COMMAND_QUEUE_DESC Desc;
};