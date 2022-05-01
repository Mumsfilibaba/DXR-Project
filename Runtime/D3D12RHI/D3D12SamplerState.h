#pragma once
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12SamplerState

class CD3D12SamplerState : public CRHISamplerState, public CD3D12DeviceChild
{
public:

    CD3D12SamplerState(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InOfflineHeap)
        : CRHISamplerState()
        , CD3D12DeviceChild(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc()
    {
        Assert(InOfflineHeap != nullptr);
    }

    ~CD3D12SamplerState()
    {
        OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
    }

    bool Init(const D3D12_SAMPLER_DESC& InDesc)
    {
        OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);
        if (OfflineHandle != 0)
        {
            Desc = InDesc;
            GetDevice()->CreateSampler(&Desc, OfflineHandle);
            return true;
        }
        else
        {
            return false;
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const { return Desc; }

private:
    CD3D12OfflineDescriptorHeap* OfflineHeap = nullptr;
    uint32                       OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    D3D12_SAMPLER_DESC           Desc;
};