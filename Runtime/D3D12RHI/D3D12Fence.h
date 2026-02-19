#pragma once
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12RefCounted.h"
#include "RHI/RHIFence.h"
#include "Core/Threading/Atomic/AtomicBool.h"

typedef TSharedRef<class FD3D12Fence>    FD3D12FenceRef;
typedef TSharedRef<class FD3D12FenceRHI> FD3D12FenceRHIRef;

class FD3D12Fence : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Fence(FD3D12Device* InDevice);
    ~FD3D12Fence();

    bool Initialize(uint64 InitialValue);

    bool WaitForValue(uint64 Value, uint32 TimeoutMs = INFINITE);

    uint64 GetCompletedValue() const
    {
        CHECK(Fence != nullptr);
        return Fence->GetCompletedValue();
    }

    void SetDebugName(const FString& Name)
    {
        CHECK(Fence != nullptr);
        Fence->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), *Name);
    }

    ID3D12Fence* GetD3D12Fence() const
    {
        return Fence.Get();
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

    bool IsReached() const
    {
        CHECK(Fence != nullptr);
        return FenceValue <= Fence->GetCompletedValue();
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

    void Release();
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
    mutable uint64 LastCompletedValue;
    uint64         CurrentValue;
    uint64         LastSignaledValue;
};

class FD3D12FenceRHI final : public FRHIFence, public FD3D12DeviceChild
{
public:
    explicit FD3D12FenceRHI(FD3D12Device* InDevice);
    virtual ~FD3D12FenceRHI() = default;

    bool Initialize();
    void Signal(ED3D12CommandQueueType QueueType);

    // FRHIFence Interface
    virtual bool IsSignaled() const override final;
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const override final;
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

private:
    FD3D12FenceRef Fence;
    uint64         CurrentValue;
    uint64         TargetValue;
    FAtomicBool    bHasPendingSignal;
    FString        DebugName;
};
