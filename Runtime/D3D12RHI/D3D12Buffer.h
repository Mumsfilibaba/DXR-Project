#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceViews.h"
class FD3D12CommandContext;

class FD3D12Buffer : public FRHIBuffer, public FD3D12GenericResource
{
public:
    static FORCEINLINE FD3D12Buffer* Cast(FRHIBuffer* Buffer)
    {
        return static_cast<FD3D12Buffer*>(Buffer);
    }

public:
    FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferInfo& InBufferInfo);
    ~FD3D12Buffer();

    bool Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    // FRHIBuffer Interface 
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(ResourceStorage.GetResource()); } 
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); } 
    
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final; 
    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final; 
    
    virtual void SetDebugName(const FString& InName) override final; 
    virtual FString GetDebugName() const override final; 

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
