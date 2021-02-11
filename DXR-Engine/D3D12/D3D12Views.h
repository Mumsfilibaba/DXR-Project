#pragma once
#include "RenderLayer/ResourceViews.h"

#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"

class D3D12Device;
class D3D12OfflineDescriptorHeap;

class D3D12View : public D3D12DeviceChild
{
public:
    D3D12View(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap);
    virtual ~D3D12View();
    
    Bool Init();

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    const D3D12Resource* GetResource() const { return Resource.Get(); }

protected:
    TRef<D3D12Resource>   Resource;
    D3D12OfflineDescriptorHeap* Heap       = nullptr;
    UInt32                      OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
};

class D3D12ConstantBufferView : public D3D12View
{
public:
    D3D12ConstantBufferView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap);
    ~D3D12ConstantBufferView() = default;

    Bool CreateView(D3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

class D3D12BaseShaderResourceView : public ShaderResourceView, public D3D12View
{
public:
    D3D12BaseShaderResourceView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap);
    ~D3D12BaseShaderResourceView() = default;

    Bool CreateView(D3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

class D3D12BaseUnorderedAccessView : public UnorderedAccessView, public D3D12View
{
public:
    D3D12BaseUnorderedAccessView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap);
    ~D3D12BaseUnorderedAccessView() = default;

    Bool CreateView(D3D12Resource* InCounterResource, D3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const { return Desc; }

    const D3D12Resource* GetCounterResource() const { return CounterResource.Get(); }

private:
    TRef<D3D12Resource> CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

class D3D12BaseRenderTargetView : public RenderTargetView, public D3D12View
{
public:
    D3D12BaseRenderTargetView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap);
    ~D3D12BaseRenderTargetView() = default;

    Bool CreateView(D3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

class D3D12BaseDepthStencilView : public DepthStencilView, public D3D12View
{
public:
    D3D12BaseDepthStencilView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap);
    ~D3D12BaseDepthStencilView() = default;

    Bool CreateView(D3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};

template<typename TBaseView>
class TD3D12BaseView : public TBaseView
{
public:
    TD3D12BaseView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
        : TBaseView(InDevice, InHeap)
    {
    }

    ~TD3D12BaseView() = default;

    virtual Bool IsValid() const override { return OfflineHandle != 0; }
};

using D3D12RenderTargetView    = TD3D12BaseView<D3D12BaseRenderTargetView>;
using D3D12DepthStencilView    = TD3D12BaseView<D3D12BaseDepthStencilView>;
using D3D12UnorderedAccessView = TD3D12BaseView<D3D12BaseUnorderedAccessView>;
using D3D12ShaderResourceView  = TD3D12BaseView<D3D12BaseShaderResourceView>;