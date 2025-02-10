#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Atomic.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RefCounted.h"

typedef TSharedRef<class FD3D12SamplerState> FD3D12SamplerStateRef;

struct FD3D12SamplerStateIdentifier
{
    static constexpr uint16 InvalidIdentifier = 0xffff;

public:
    enum class EGenerate
    {
        New
    };

    FD3D12SamplerStateIdentifier()
        : Identifer(InvalidIdentifier)
    {
    }

    FD3D12SamplerStateIdentifier(EGenerate Type)
        : Identifer(GenerateIdentifier())
    {
    }

    operator bool() const
    {
        return Identifer != InvalidIdentifier;
    }

    uint16 operator*() const
    {
        return Identifer;
    }

    bool operator==(const FD3D12SamplerStateIdentifier& Other) const
    {
        return Identifer == Other.Identifer;
    }

    bool operator!=(const FD3D12SamplerStateIdentifier& Other) const
    {
        return Identifer != Other.Identifer;
    }

    uint16 Identifer;

private:
    static uint16 GenerateIdentifier()
    {
        const int32 Identifier = ++NextIdentifier;
        CHECK(Identifier < InvalidIdentifier);
        return static_cast<uint16>(Identifier);
    }

    static D3D12RHI_API FAtomicInt32 NextIdentifier;
};


class FD3D12SamplerState : public FRHISamplerState, public FD3D12DeviceChild
{
public:
    FD3D12SamplerState(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, const FRHISamplerStateInfo& InSamplerInfo);
    virtual ~FD3D12SamplerState();

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

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
};
