#pragma once
#include "D3D12Resource.h"
#include "D3D12ResourceViews.h"
#include "D3D12RefCounted.h"
#include "RHI/RHIResources.h"

class FD3D12Buffer : public FRHIBuffer, public FD3D12DeviceChild
{
public:
    FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferDesc& InDesc);
    ~FD3D12Buffer();

    bool Initialize(EResourceAccess InInitialAccess, const void* InInitialData);

    virtual void* GetRHIBaseBuffer()         override final { return reinterpret_cast<void*>(static_cast<FD3D12Buffer*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final;
    virtual FString GetName() const override final;

    void SetResource(FD3D12Resource* InResource);
    
    FD3D12ConstantBufferView* GetConstantBufferView() const
    {
        return View.Get();
    }

    FD3D12Resource* GetD3D12Resource() const 
    {
        return Resource.Get();
    }

private:
    bool CreateCBV();

    FD3D12ResourceRef           Resource;
    FD3D12ConstantBufferViewRef View;
};

inline FD3D12Buffer* GetD3D12Buffer(FRHIBuffer* Buffer)
{
    return static_cast<FD3D12Buffer*>(Buffer);
}

inline FD3D12Resource* GetD3D12Resource(FRHIBuffer* Buffer)
{
    return Buffer ? static_cast<FD3D12Buffer*>(Buffer)->GetD3D12Resource() : nullptr;
}