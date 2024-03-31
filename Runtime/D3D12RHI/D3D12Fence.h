#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"

typedef TSharedRef<class FD3D12Fence> FD3D12FenceRef;

class FD3D12Fence : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Fence(FD3D12Device* InDevice);
    ~FD3D12Fence();

    bool Initialize(uint64 InitalValue);

    bool WaitForValue(uint64 Value);

    FORCEINLINE ID3D12Fence* GetD3D12Fence() const { return Fence.Get(); }

    FORCEINLINE void SetDebugName(const FString& Name)
    {
        CHECK(Fence != nullptr);
        Fence->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.GetCString());
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE               Event;
};


struct FD3D12FenceSyncPoint
{
    FD3D12FenceSyncPoint()
        : Fence(nullptr)
        , FenceValue(0)
    {
    }

    FD3D12FenceSyncPoint(FD3D12Fence* InFence, uint64 InFenceValue)
        : Fence(InFence)
        , FenceValue(InFenceValue)
    {
    }

    FD3D12Fence* Fence;
    uint64       FenceValue;
};


class FD3D12FenceManager : public FD3D12DeviceChild
{
public:
    FD3D12FenceManager(FD3D12Device* InDevice);
    ~FD3D12FenceManager() = default;

    bool Initialize();

    uint64 SignalGPU(ED3D12CommandQueueType QueueType);

    void WaitGPU(ED3D12CommandQueueType QueueType);
    
    void WaitGPU(ED3D12CommandQueueType QueueType, uint64 InFenceValue);

    void WaitForFence();

    void WaitForFence(uint64 InFenceValue);

    uint64 GetCompletedValue() const;
    
    FD3D12Fence* GetFence() const 
    {
        return Fence.Get();
    }

    uint64 GetLastSignaledValue() const 
    { 
        return LastSignaledValue; 
    }

    uint64 GetCurrentValue() const 
    { 
        return CurrentValue; 
    }

    uint64 GetCompletedValueFast() const 
    { 
        return LastCompletedValue;
    }

private:
    FD3D12FenceRef Fence;

    uint64         CurrentValue;
    uint64         LastSignaledValue;
    mutable uint64 LastCompletedValue;
};