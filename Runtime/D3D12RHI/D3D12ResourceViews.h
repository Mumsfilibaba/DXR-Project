#pragma once
#include "Core/RefCountedBase.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Resource.h"

class FD3D12OfflineDescriptorHeap;

typedef TSharedRef<class FD3D12ConstantBufferView>  FD3D12ConstantBufferViewRef;
typedef TSharedRef<class FD3D12ShaderResourceView>  FD3D12ShaderResourceViewRef;
typedef TSharedRef<class FD3D12UnorderedAccessView> FD3D12UnorderedAccessViewRef;
typedef TSharedRef<class FD3D12RenderTargetView>    FD3D12RenderTargetViewRef;
typedef TSharedRef<class FD3D12DepthStencilView>    FD3D12DepthStencilViewRef;

class FD3D12View : public FD3D12DeviceChild, public ID3D12ResourceRelocationListener
{
public:
    FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12View();

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool AllocateHandle();
    void InvalidateAndFreeHandle();

    void RegisterWithResource(FD3D12GenericResource* InOwner);
    void UnregisterFromResource();

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return Descriptor.Handle;
    }

    const FD3D12Resource* GetViewResource() const 
    { 
        return ViewResource.Get(); 
    }

    FD3D12ResidencyHandle* GetResourceResidencyHandle() const
    {
        FD3D12Resource* Resource = ViewResource.Get();
        return Resource ? Resource->GetResidencyHandle() : nullptr;
    }

    uint32 GetDescriptorVersion() const
    {
        return DescriptorVersion;
    }

protected:
    void IncrementDescriptorVersion()
    {
        ++DescriptorVersion;
    }

    FD3D12ResourceRef            ViewResource;
    FD3D12OfflineDescriptorHeap& OfflineHeap;
    FD3D12OfflineDescriptor      Descriptor;
    FD3D12GenericResource*       OwnerResource;
    uint32                       DescriptorVersion;
};

class FD3D12ConstantBufferView : public FD3D12View, public FRefCountedBase
{
public:
    FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12ConstantBufferView() = default;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool CreateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const 
    {
        return Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

class FD3D12ShaderResourceView : public FRHIShaderResourceView, public FD3D12View
{
public:
    FD3D12ShaderResourceView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource);
    virtual ~FD3D12ShaderResourceView() = default;

    // FRHIShaderResourceView Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool CreateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

class FD3D12UnorderedAccessView : public FRHIUnorderedAccessView, public FD3D12View
{
public:
    FD3D12UnorderedAccessView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource);
    virtual ~FD3D12UnorderedAccessView() = default;
 
    // FRHIUnorderedAccessView Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool CreateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const
    { 
        return Desc;
    }

    const FD3D12Resource* GetCounterResource() const
    { 
        return CounterResource.Get(); 
    }

private:
    FD3D12ResourceRef                CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

class FD3D12RenderTargetView : public FD3D12View, public FRefCountedBase
{
public:
    FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12RenderTargetView() = default;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const 
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

class FD3D12DepthStencilView : public FD3D12View, public FRefCountedBase
{
public:
    FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12DepthStencilView() = default;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const 
    { 
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};
