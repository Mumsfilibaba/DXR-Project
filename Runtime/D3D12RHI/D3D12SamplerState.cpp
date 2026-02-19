#include "D3D12RHI/D3D12SamplerState.h"

FAtomicInt32 FD3D12SamplerStateIdentifier::NextIdentifier = 0;

FD3D12SamplerStateRHI::FD3D12SamplerStateRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FD3D12DeviceChild(InDevice)
    , D3D12Desc()
    , OfflineHeap(InOfflineHeap)
    , Descriptor()
    , Identifier(FD3D12SamplerStateIdentifier::EGenerate::New)
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

FD3D12SamplerStateRHI::~FD3D12SamplerStateRHI()
{
	if (Descriptor)
	{
	    OfflineHeap.Free(Descriptor);
        Descriptor = {};
    }
}

bool FD3D12SamplerStateRHI::CreateSampler(const D3D12_SAMPLER_DESC& InDesc)
{
    Descriptor = OfflineHeap.Allocate();
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("Failed to allocate DescriptorHandle for SamplerState");
        return false;
    }

    GetDevice()->GetD3D12Device()->CreateSampler(&InDesc, GetOfflineHandle());
    D3D12Desc = InDesc;
    return true;
}
