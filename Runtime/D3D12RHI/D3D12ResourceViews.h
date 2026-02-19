#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12RefCounted.h"

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

    bool AllocateHandle();
    void InvalidateAndFreeHandle();

    void RegisterWithResource(FD3D12BaseResource* InOwner);
    void UnregisterFromResource();

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return Descriptor.Handle;
    }

    const FD3D12Resource* GetViewResource() const 
    { 
        return ViewResource.Get(); 
    }

    uint32 GetDescriptorVersion() const
    {
        return DescriptorVersion;
    }

protected:
    void IncrementDescriptorVersion() { ++DescriptorVersion; }

    FD3D12ResourceRef            ViewResource;
    FD3D12OfflineDescriptorHeap& OfflineHeap;
    FD3D12OfflineDescriptor      Descriptor;
    FD3D12BaseResource*          OwnerResource = nullptr;
    uint32                       DescriptorVersion = 0;
};

class FD3D12ConstantBufferView : public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);

    virtual void OnRelocation(FD3D12BaseResource* Resource) override;

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

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
    virtual void OnRelocation(FD3D12BaseResource* Resource) override;

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

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
    virtual void OnRelocation(FD3D12BaseResource* Resource) override;

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

class FD3D12RenderTargetView : public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12RenderTargetView() = default;

    virtual void OnRelocation(FD3D12BaseResource* Resource) override;

    bool CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const 
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

class FD3D12DepthStencilView : public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12DepthStencilView() = default;

    virtual void OnRelocation(FD3D12BaseResource* Resource) override;

    bool CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const 
    { 
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};
