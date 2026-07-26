#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceViews.h"
class FD3D12CommandContext;

typedef TSharedRef<class FD3D12BufferRHI> FD3D12BufferRHIRef;

class FD3D12BufferRHI : public FRHIBuffer, public FD3D12ResourceBase
{
public:
    static uint64 GetBufferAlignment(const FRHIBufferDesc& Desc);
    
public:
    FD3D12BufferRHI(FD3D12Device* InDevice, const FRHIBufferDesc& InBufferDesc);
    ~FD3D12BufferRHI();

    // FRHIBuffer Interface
    virtual void* GetRHINativeResource() const override final;
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;
    
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX)   override final; 
    virtual void  Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final; 
    
    virtual void SetDebugName(const String& InName)       override final; 
    virtual void GetDebugName(String& OutDebugName) const override final; 
    
    bool Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    void SetResource(FD3D12Resource* InResource); 
    
    FD3D12ConstantBufferView* GetOrCreateConstantBufferView();

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return ResourceStorage.GetGPUVirtualAddress();
    }

private:
    bool CreateConstantBufferView();

    FD3D12ConstantBufferViewRef ConstantBufferView;
};
