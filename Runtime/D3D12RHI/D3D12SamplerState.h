#pragma once
#include "D3D12Descriptors.h"
#include "D3D12Device.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12SamplerState

class FD3D12SamplerState : public FRHISamplerState, public FD3D12DeviceChild
{
public:

    FD3D12SamplerState(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InOfflineHeap)
        : FRHISamplerState()
        , FD3D12DeviceChild(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc()
    {
        Check(InOfflineHeap != nullptr);
    }

    ~FD3D12SamplerState()
    {
        OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
    }

public:

    bool CreateSampler(const D3D12_SAMPLER_DESC& InDesc)
    {
        OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);
        if (OfflineHandle != 0)
        {
            Desc = InDesc;
            GetDevice()->GetD3D12Device()->CreateSampler(&Desc, OfflineHandle);
            return true;
        }
        else
        {
            return false;
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const { return Desc; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHISamplerState Interface

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

private:
    FD3D12OfflineDescriptorHeap* OfflineHeap      = nullptr;
    uint32                       OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    D3D12_SAMPLER_DESC           Desc;
};