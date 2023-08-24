#include "D3D12SamplerState.h"

FD3D12SamplerState::FD3D12SamplerState(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InOfflineHeap, const FRHISamplerStateDesc& InInitializer)
    : FRHISamplerState(InInitializer)
    , FD3D12DeviceChild(InDevice)
    , OfflineHeap(InOfflineHeap)
    , OfflineHandle({ 0 })
    , Desc()
{
    CHECK(InOfflineHeap != nullptr);
}

FD3D12SamplerState::~FD3D12SamplerState()
{
    CHECK(OfflineHeap != nullptr);
    OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
}

bool FD3D12SamplerState::CreateSampler(const D3D12_SAMPLER_DESC& InDesc)
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