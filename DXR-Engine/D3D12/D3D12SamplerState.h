#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

class D3D12SamplerState : public SamplerState, public D3D12DeviceChild
{
public:
    D3D12SamplerState(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InOfflineHeap)
        : SamplerState()
        , D3D12DeviceChild(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc()
    {
        Assert(InOfflineHeap !=  nullptr);
    }

    ~D3D12SamplerState()
    {
        OfflineHeap->Free(OfflineHandle, OfflineHeapIndex);
    }

    bool Init(const D3D12_SAMPLER_DESC& InDesc)
    {
        OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);
        if (OfflineHandle != 0)
        {
            Desc = InDesc;
            GetDevice()->CreateSampler(&Desc, OfflineHandle);
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(OfflineHandle.ptr);
    }

    virtual bool IsValid() const override { return OfflineHandle != 0; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    const D3D12_SAMPLER_DESC& GetDesc() const { return Desc; }

private:
    D3D12OfflineDescriptorHeap* OfflineHeap      = nullptr;
    uint32                      OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
    D3D12_SAMPLER_DESC          Desc;
};