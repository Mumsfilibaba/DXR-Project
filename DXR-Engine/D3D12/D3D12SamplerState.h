#pragma once
#include "RenderLayer/SamplerState.h"

#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

class D3D12SamplerState : public SamplerState, public D3D12DeviceChild
{
public:
    D3D12SamplerState(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InOfflineHeap, const D3D12_SAMPLER_DESC& InDesc)
        : SamplerState()
        , D3D12DeviceChild(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc(InDesc)
    {
        VALIDATE(InOfflineHeap !=  nullptr);
        OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);

        CreateView(Desc);
    }

    ~D3D12SamplerState()
    {
        OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
    }

    FORCEINLINE void CreateView(const D3D12_SAMPLER_DESC& InDesc)
    {
        Desc = InDesc;
        Device->CreateSampler(&Desc, OfflineHandle);
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
    D3D12_SAMPLER_DESC Desc;
};