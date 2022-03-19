#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CD3D12Buffer> CD3D12BufferRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Buffer

class CD3D12Buffer : public CRHIBuffer, public CD3D12DeviceObject
{
public:
    CD3D12Buffer(CD3D12Device* InDevice, const CRHIBufferDesc& InBufferDesc);
    ~CD3D12Buffer() = default;

    bool Initialize(class CD3D12CommandContext* CommandContext, ERHIResourceState InitalState, const CRHIResourceData* InitialData);

    virtual void SetName(const String& InName) override final;

    virtual void* GetNativeResource() const override final;

    virtual bool IsValid() const override final;

    void SetResource(const CD3D12ResourceRef& InResource);

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return static_cast<uint64>(Resource->GetDesc().Width);
    }

    FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return Resource ? Resource->GetGPUVirtualAddress() : 0U;
    }

    FORCEINLINE CD3D12Resource* GetResource() const
    {
        return Resource.Get();
    }

    FORCEINLINE CD3D12ConstantBufferView* GetConstantBufferView() const
    {
        return ConstantBufferView.Get();
    }

protected:
    bool CreateConstantBufferView();

    CD3D12ResourceRef           Resource;
    CD3D12ConstantBufferViewRef ConstantBufferView;
};
