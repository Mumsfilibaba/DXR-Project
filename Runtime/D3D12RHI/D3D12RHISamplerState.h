#pragma once
#include "RHI/RHIResources.h"

#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHISamplerState

class CD3D12RHISamplerState : public CRHISamplerState, public CD3D12DeviceObject
{
public:

    CD3D12RHISamplerState(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InOfflineHeap)
        : CRHISamplerState()
        , CD3D12DeviceObject(InDevice)
        , OfflineHeap(InOfflineHeap)
        , OfflineHandle({ 0 })
        , Desc()
    {
        Assert(InOfflineHeap != nullptr);
    }

    ~CD3D12RHISamplerState()
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

    virtual bool IsValid() const override
    {
        return OfflineHandle != 0;
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return OfflineHandle;
    }

    FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    CD3D12OfflineDescriptorHeap* OfflineHeap = nullptr;
    uint32                      OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
    D3D12_SAMPLER_DESC          Desc;
};