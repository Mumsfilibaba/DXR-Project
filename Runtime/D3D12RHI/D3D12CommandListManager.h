#pragma once
#include "D3D12CommandList.h"

class FD3D12Device;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandListManager

class FD3D12CommandListManager
    : public FD3D12DeviceChild
{
public:
    FD3D12CommandListManager(FD3D12Device* InDevice);
    ~FD3D12CommandListManager();

    FD3D12CommandList* ObtainCommandList();
    void ReleaseCommandList(FD3D12CommandList* InCommandList);

    void ExecuteCommandList(FD3D12CommandList* InCommandList);

    FORCEINLINE ID3D12CommandQueue* GetD3D12CommandQueue() { return CommandQueue.Get(); }

private:
    TComPtr<ID3D12CommandQueue> CommandQueue;
    TArray<FD3D12CommandList*>  CommandLists;
};