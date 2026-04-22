#pragma once
#include "Core/RefCountedBase.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "RHI/RHIFence.h"
#include "D3D12RHI/D3D12DeviceChild.h"

typedef TSharedRef<class FD3D12Fence>    FD3D12FenceRef;
typedef TSharedRef<class FD3D12FenceRHI> FD3D12FenceRHIRef;

class FD3D12Fence : public FD3D12DeviceChild, public FRefCountedBase
{
public:
    FD3D12Fence(FD3D12Device* InDevice);
    ~FD3D12Fence();

    bool Initialize(uint64 InitialValue = 0);

    uint64 Signal(ID3D12CommandQueue* Queue);
    void   WaitGPU(ID3D12CommandQueue* Queue, uint64 Value);
    bool   WaitForValue(uint64 Value, uint32 TimeoutMs = INFINITE);

    uint64 GetCompletedValue() const;

    void SetDebugName(const FString& Name);

    uint64 GetCurrentValue()      const { return CurrentValue; }
    uint64 GetLastSignaledValue() const { return LastSignaledValue; }

    ID3D12Fence* GetD3D12Fence() const
    {
        return Fence.Get();
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE               Event;
    mutable uint64       LastCompletedValue;
    uint64               CurrentValue;
    uint64               LastSignaledValue;
};

class FD3D12FenceSyncPoint
{
public:
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

    bool IsValid() const
    {
        return Fence != nullptr;
    }

    bool IsReached() const
    {
        CHECK(Fence != nullptr);
        return FenceValue <= Fence->GetCompletedValue();
    }

    bool Wait(uint32 TimeoutMs = INFINITE) const
    {
        CHECK(Fence != nullptr);
        return Fence->WaitForValue(FenceValue, TimeoutMs);
    }

    uint64 GetFenceValue() const
    {
        return FenceValue;
    }

    FD3D12Fence* GetFence() const
    {
        return Fence;
    }

private:
    FD3D12Fence* Fence;
    uint64       FenceValue;
};

class FD3D12FenceRHI final : public FRHIFence, public FD3D12DeviceChild
{
public:
    explicit FD3D12FenceRHI(FD3D12Device* InDevice);
    virtual ~FD3D12FenceRHI() = default;

    bool Initialize();
    void Signal(ID3D12CommandQueue* Queue);

    // FRHIFence Interface
    virtual bool IsSignaled() const override final;
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const override final;

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

private:
    FD3D12FenceRef Fence;
    FAtomicBool    bHasPendingSignal;
    FString        DebugName;
};
