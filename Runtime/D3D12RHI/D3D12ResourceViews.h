#pragma once
#include "Core/RefCountedBase.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Resource.h"

class FD3D12OfflineDescriptorHeap;
class FD3D12SwapChainRHI;
class FD3D12BackBufferProxyTextureRHI;

typedef TSharedRef<class FD3D12ConstantBufferView>                 FD3D12ConstantBufferViewRef;
typedef TSharedRef<class FD3D12ShaderResourceViewRHI>              FD3D12ShaderResourceViewRHIRef;
typedef TSharedRef<class FD3D12UnorderedAccessViewRHI>             FD3D12UnorderedAccessViewRHIRef;
typedef TSharedRef<class FD3D12RenderTargetViewRHI>                FD3D12RenderTargetViewRHIRef;
typedef TSharedRef<class FD3D12DepthStencilViewRHI>                FD3D12DepthStencilViewRHIRef;
typedef TSharedRef<class FD3D12BackBufferProxyRenderTargetViewRHI> FD3D12BackBufferProxyRenderTargetViewRHIRef;

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

    FD3D12Resource* GetViewResource()
    {
        return ViewResource.Get();
    }

    FD3D12ResidencyHandle* GetResourceResidencyHandle() const
    {
        FD3D12Resource* Resource = ViewResource.Get();
        return Resource ? Resource->GetResidencyHandle() : nullptr;
    }

    void ReleaseViewResource()
    {
        ViewResource = nullptr;
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

class FD3D12ShaderResourceViewRHI : public FRHIShaderResourceView, public FD3D12View
{
public:
    FD3D12ShaderResourceViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource);
    virtual ~FD3D12ShaderResourceViewRHI() = default;

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

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

class FD3D12UnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FD3D12View
{
public:
    FD3D12UnorderedAccessViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource);
    virtual ~FD3D12UnorderedAccessViewRHI() = default;
 
    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

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

class FD3D12RenderTargetViewBase : public FRHIRenderTargetView
{
protected:
    explicit FD3D12RenderTargetViewBase(FRHIResource* InResource)
        : FRHIRenderTargetView(InResource)
    {
    }

    virtual ~FD3D12RenderTargetViewBase() = default;

public:
    virtual FD3D12RenderTargetViewRHI* GetRenderTargetViewInterface() const = 0;
};

class FD3D12RenderTargetViewRHI : public FD3D12RenderTargetViewBase, public FD3D12View
{
public:
    FD3D12RenderTargetViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource);
    virtual ~FD3D12RenderTargetViewRHI() = default;

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    // FD3D12RenderTargetViewBase Interface
    virtual FD3D12RenderTargetViewRHI* GetRenderTargetViewInterface() const override;

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

class FD3D12BackBufferProxyRenderTargetViewRHI : public FD3D12RenderTargetViewBase
{
public:
    FD3D12BackBufferProxyRenderTargetViewRHI(FD3D12SwapChainRHI* InSwapChain, FD3D12BackBufferProxyTextureRHI* InProxyTexture);
    virtual ~FD3D12BackBufferProxyRenderTargetViewRHI();

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    // FD3D12RenderTargetViewBase Interface
    virtual FD3D12RenderTargetViewRHI* GetRenderTargetViewInterface() const override final;

    void SetSwapChain(FD3D12SwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

    FD3D12SwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

private:
    FD3D12SwapChainRHI* SwapChain;
};

class FD3D12DepthStencilViewRHI : public FRHIDepthStencilView, public FD3D12View
{
public:
    FD3D12DepthStencilViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource);
    virtual ~FD3D12DepthStencilViewRHI() = default;

    // FRHIDepthStencilView Interface
    virtual void* GetRHINativeHandle() const override final;

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
