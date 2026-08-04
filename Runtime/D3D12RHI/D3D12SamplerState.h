#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Atomic.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12Device.h"
typedef TSharedRef<class FD3D12SamplerStateRHI> FD3D12SamplerStateRHIRef;

struct FD3D12SamplerStateIdentifier
{
    static constexpr uint16 InvalidIdentifier = 0xffff;

public:
    enum class EGenerate : uint8
    {
        New,
    };

    FD3D12SamplerStateIdentifier()
        : Identifier(InvalidIdentifier)
    {
    }

    FD3D12SamplerStateIdentifier(EGenerate Type)
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

    bool operator==(const FD3D12SamplerStateIdentifier& Other) const
    {
        return Identifier == Other.Identifier;
    }

    bool operator!=(const FD3D12SamplerStateIdentifier& Other) const
    {
        return Identifier != Other.Identifier;
    }

    uint16 Identifier;

private:
    static uint16 GenerateIdentifier();

    static D3D12RHI_API AtomicInt32 NextIdentifier;
};


class FD3D12SamplerStateRHI : public FRHISamplerState, public FD3D12DeviceChild
{
public:
    FD3D12SamplerStateRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, const FRHISamplerStateDesc& InSamplerDesc);
    virtual ~FD3D12SamplerStateRHI();

    // FRHISamplerState Interface
    virtual void* GetRHINativeSampler() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    bool CreateSampler(const D3D12_SAMPLER_DESC& InDesc);

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const 
    {
        return Descriptor.Handle;
    }

    const D3D12_SAMPLER_DESC& GetDesc() const 
    { 
        return Desc;
    }

    FD3D12SamplerStateIdentifier GetUniqueID() const
    {
        return Identifier;
    }

private:
    D3D12_SAMPLER_DESC           Desc;
    FD3D12OfflineDescriptorHeap& OfflineHeap;
    FD3D12OfflineDescriptor      Descriptor;
    FD3D12SamplerStateIdentifier Identifier;
    mutable FRHIDescriptorHandle BindlessHandle;
};
