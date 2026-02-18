#pragma once
#include "RHI/RHIResources.h"
#include "Core/Containers/UniquePtr.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceState.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12RefCounted.h"

class FD3D12CommandContext;

class FD3D12Buffer : public FRHIBuffer, public FD3D12DeviceChild
{
public:
    FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferInfo& InBufferInfo);
    ~FD3D12Buffer();

    bool Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    // FRHIBuffer Interface 
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetResource()); } 
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); } 
    
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final; 
    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final; 
    
    virtual void SetDebugName(const FString& InName) override final; 
    virtual FString GetDebugName() const override final; 

    void SetResource(FD3D12Resource* InResource); 

    void EnableStateTracking(EResourceAccess InitialState);
    void DisableStateTracking(FD3D12CommandContext* CommandContext = nullptr);
    
    FD3D12ConstantBufferView* GetConstantBufferView() const
    {
        return ConstantBufferView.Get();
    }

    FD3D12Resource* GetResource() const 
    {
        return Resource.Get();
    }

    FD3D12ResourceState* GetResourceState() const
    {
        return ResourceState.Get();
    }

private:
    bool CreateCBV();

    FD3D12ResourceRef               Resource;
    FD3D12ConstantBufferViewRef     ConstantBufferView;
    TUniquePtr<FD3D12ResourceState> ResourceState;
};
