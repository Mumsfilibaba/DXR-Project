#include "D3D12RHI/D3D12SamplerState.h"

FAtomicInt32 FD3D12SamplerStateIdentifier::NextIdentifier = 0;

FD3D12SamplerState::FD3D12SamplerState(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, const FRHISamplerStateInfo& InSamplerInfo)
    : FRHISamplerState(InSamplerInfo)
    , FD3D12DeviceChild(InDevice)
    , Desc()
    , OfflineHeap(InOfflineHeap)
    , Descriptor()
    , Identifier(FD3D12SamplerStateIdentifier::EGenerate::New)
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

FD3D12SamplerState::~FD3D12SamplerState()
{
    OfflineHeap.Free(Descriptor);
}

bool FD3D12SamplerState::CreateSampler(const D3D12_SAMPLER_DESC& InDesc)
{
    Descriptor = OfflineHeap.Allocate();
    if (!Descriptor)
    {
        D3D12_ERROR("Failed to allocate DescriptorHandle for SamplerState");
        return false;
    }

    GetDevice()->GetD3D12Device()->CreateSampler(&InDesc, GetOfflineHandle());
    Desc = InDesc;
    return true;
}