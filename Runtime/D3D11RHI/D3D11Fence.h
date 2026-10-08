#pragma once
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "RHI/RHIFence.h"
#include "D3D11RHI/D3D11DeviceChild.h"
#include "D3D11RHI/D3D11Query.h"

typedef TSharedRef<class FD3D11FenceRHI> FD3D11FenceRHIRef;

class FD3D11FenceRHI final : public FRHIFence, public FD3D11DeviceChild
{
public:
    FD3D11FenceRHI(FD3D11Device* InDevice);
    virtual ~FD3D11FenceRHI();

    // FRHIFence Interface
    virtual void* GetRHINativeFence() const override final;

    virtual bool IsSignaled()                        const override final;
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();
    void Signal();

private:
    TComPtr<ID3D11Fence> Fence;
    FD3D11QueryRef       EventQuery;
    HANDLE               Event;
    AtomicUInt64         SignaledValue;
    AtomicBool           bHasPendingSignal;
};
