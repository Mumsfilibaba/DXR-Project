#include "D3D12RHI/D3D12SamplerState.h"

FAtomicInt32 FD3D12SamplerStateIdentifier::NextIdentifier = 0;

uint16 FD3D12SamplerStateIdentifier::GenerateIdentifier()
{
    const int32 Counter = ++NextIdentifier;
    CHECK(Counter < InvalidIdentifier);

    // Bijective scramble so that sequential counters produce well-distributed
    // uint16 values, avoiding CRC32 hash clustering in the sampler cache.
    uint16 x = static_cast<uint16>(Counter);
    x *= 0xA3B1;
    x ^= (x >> 7);
    x *= 0x27D5;
    x ^= (x >> 9);

    return (x != InvalidIdentifier) ? x : 0;
}

FD3D12SamplerStateRHI::FD3D12SamplerStateRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FD3D12DeviceChild(InDevice)
    , Desc()
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

void* FD3D12SamplerStateRHI::GetRHINativeSampler() const
{
    return reinterpret_cast<void*>(static_cast<UPTR_INT>(Descriptor.Handle.ptr));
}

FRHIDescriptorHandle FD3D12SamplerStateRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
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
    Desc = InDesc;
    return true;
}
