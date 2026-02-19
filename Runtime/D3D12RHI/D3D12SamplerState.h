#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Atomic.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RefCounted.h"

typedef TSharedRef<class FD3D12SamplerStateRHI> FD3D12SamplerStateRHIRef;

struct FD3D12SamplerStateRHIIdentifier
{
    static constexpr uint16 InvalidIdentifier = 0xffff;

public:
    enum class EGenerate
    {
        New
    };

    FD3D12SamplerStateRHIIdentifier()
        : Identifier(InvalidIdentifier)
    {
    }

    FD3D12SamplerStateRHIIdentifier(EGenerate Type)
        : Identifier(GenerateIdentifier())
    {
    }

    operator bool() const
    {
        return Identifier != InvalidIdentifier;
    }

    uint16 operator*() const
    {
        return Identifier;
    }

    bool operator==(const FD3D12SamplerStateRHIIdentifier& Other) const
    {
        return Identifier == Other.Identifier;
    }

    bool operator!=(const FD3D12SamplerStateRHIIdentifier& Other) const
    {
        return Identifier != Other.Identifier;
    }

    uint16 Identifier;

private:
    static uint16 GenerateIdentifier()
    {
        const int32 Identifier = ++NextIdentifier;
        CHECK(Identifier < InvalidIdentifier);
        return static_cast<uint16>(Identifier);
    }

    static D3D12RHI_API FAtomicInt32 NextIdentifier;
};


class FD3D12SamplerStateRHI : public FRHISamplerState, public FD3D12DeviceChild
{
public:
    FD3D12SamplerStateRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, const FRHISamplerStateDesc& InSamplerDesc);
    virtual ~FD3D12SamplerStateRHI();

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool CreateSampler(const D3D12_SAMPLER_DESC& InDesc);

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const 
    {
        return Descriptor.Handle;
    }

    const D3D12_SAMPLER_DESC& GetDesc() const 
    { 
        return D3D12Desc;
    }

    FD3D12SamplerStateRHIIdentifier GetUniqueID() const
    {
        return Identifier;
    }

private:
    D3D12_SAMPLER_DESC           D3D12Desc;
    FD3D12OfflineDescriptorHeap& OfflineHeap;
    FD3D12OfflineDescriptor      Descriptor;
    FD3D12SamplerStateRHIIdentifier Identifier;
};
