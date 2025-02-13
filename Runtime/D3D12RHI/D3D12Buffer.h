#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12RefCounted.h"

class FD3D12CommandContext;

class FD3D12Buffer : public FRHIBuffer, public FD3D12DeviceChild
{
public:
    FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferInfo& InBufferInfo);
    ~FD3D12Buffer();

    bool Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

public:

    // FRHIBuffer Interface
    virtual void* GetRHINativeHandle() const { return reinterpret_cast<void*>(GetD3D12Resource()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

public:
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