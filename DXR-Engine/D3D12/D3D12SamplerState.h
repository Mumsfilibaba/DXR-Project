#pragma once
#include "RenderLayer/SamplerState.h"

#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

class D3D12SamplerState : public SamplerState, public D3D12DeviceChild
{
public:
    D3D12SamplerState(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InOfflineHeap)
        : SamplerState()
        , D3D12DeviceChild(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc()
    {
        VALIDATE(InOfflineHeap !=  nullptr);
    }

    ~D3D12SamplerState()
    {
        OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
    }

    Bool Init()
    {
        OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);
        return OfflineHandle != 0;
    }

    FORCEINLINE Bool CreateView(const D3D12_SAMPLER_DESC& InDesc)
    {
        VALIDATE(OfflineHandle != 0);

        Desc = InDesc;
        Device->CreateSampler(&Desc, OfflineHandle);

        return true;
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return OfflineHandle;
    }

    FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12OfflineDescriptorHeap* OfflineHeap      = nullptr;
    UInt32                      OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
    D3D12_SAMPLER_DESC          Desc;
};