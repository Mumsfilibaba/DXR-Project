#pragma once
#include "D3D12Descriptors.h"
#include "D3D12Device.h"
#include "D3D12RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FD3D12SamplerState> FD3D12SamplerStateRef;

class FD3D12SamplerState : public FRHISamplerState, public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12SamplerState(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InOfflineHeap, const FRHISamplerStateDesc& InInitializer);
    virtual ~FD3D12SamplerState();

    bool CreateSampler(const D3D12_SAMPLER_DESC& InDesc);

    virtual int32 AddRef() override final { return FD3D12RefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FD3D12RefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const 
    {
        return OfflineHandle;
    }

    FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const 
    { 
        return Desc;
    }

private:
    FD3D12OfflineDescriptorHeap* OfflineHeap      = nullptr;
    uint32                       OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    D3D12_SAMPLER_DESC           Desc;
};