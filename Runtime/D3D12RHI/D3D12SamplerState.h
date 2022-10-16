#pragma once
#include "D3D12Descriptors.h"
#include "D3D12Device.h"

#include "RHI/RHIResources.h"

#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FD3D12SamplerState> FD3D12SamplerStateRef;

class FD3D12SamplerState 
    : public FRHISamplerState
    , public FD3D12DeviceChild
{
public:
    FD3D12SamplerState(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InOfflineHeap, const FRHISamplerStateInitializer& InInitializer)
        : FRHISamplerState(InInitializer)
        , FD3D12DeviceChild(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc()
    {
        CHECK(InOfflineHeap != nullptr);
    }

    ~FD3D12SamplerState()
    {
        OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    bool CreateSampler(const D3D12_SAMPLER_DESC& InDesc)
    {
        OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);
        if (OfflineHandle == 0)
        {
            D3D12_ERROR("Failed to allocate DescriptorHandle for SamplerState");
            return false;
        }

        GetDevice()->GetD3D12Device()->CreateSampler(&InDesc, OfflineHandle);
        Desc = InDesc;

        return true;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const { return Desc; }

private:
    FD3D12OfflineDescriptorHeap* OfflineHeap      = nullptr;
    uint32                       OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    D3D12_SAMPLER_DESC           Desc;
};