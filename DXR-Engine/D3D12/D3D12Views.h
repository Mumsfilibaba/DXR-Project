#pragma once
#include "RHICore/RHIResourceViews.h"

#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"

class CD3D12Device;
class CD3D12OfflineDescriptorHeap;

class CD3D12View : public CD3D12DeviceChild
{
public:
    CD3D12View( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap );
    virtual ~CD3D12View();

    bool Init();

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return OfflineHandle;
    }

    FORCEINLINE const D3D12Resource* GetResource() const
    {
        return Resource.Get();
    }

protected:
    TSharedRef<D3D12Resource> Resource;
    CD3D12OfflineDescriptorHeap* Heap = nullptr;
    uint32                      OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
};

class CD3D12ConstantBufferView : public CD3D12View
{
public:
    CD3D12ConstantBufferView( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap );
    ~CD3D12ConstantBufferView() = default;

    bool CreateView( D3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc );

    FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

class CD3D12BaseShaderResourceView : public CRHIShaderResourceView, public CD3D12View
{
public:
    CD3D12BaseShaderResourceView( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap );
    ~CD3D12BaseShaderResourceView() = default;

    bool CreateView( D3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc );

    FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

class CD3D12BaseUnorderedAccessView : public CRHIUnorderedAccessView, public CD3D12View
{
public:
    CD3D12BaseUnorderedAccessView( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap );
    ~CD3D12BaseUnorderedAccessView() = default;

    bool CreateView( D3D12Resource* InCounterResource, D3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc );

    FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

    FORCEINLINE const D3D12Resource* GetCounterResource() const
    {
        return CounterResource.Get();
    }

private:
    TSharedRef<D3D12Resource> CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

class CD3D12BaseRenderTargetView : public CRHIRenderTargetView, public CD3D12View
{
public:
    CD3D12BaseRenderTargetView( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap );
    ~CD3D12BaseRenderTargetView() = default;

    bool CreateView( D3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc );

    FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

class CD3D12BaseDepthStencilView : public CRHIDepthStencilView, public CD3D12View
{
public:
    CD3D12BaseDepthStencilView( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap );
    ~CD3D12BaseDepthStencilView() = default;

    bool CreateView( D3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc );

    FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};

template<typename TBaseView>
class TD3D12BaseView : public TBaseView
{
public:
    TD3D12BaseView( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap )
        : TBaseView( InDevice, InHeap )
    {
    }

    ~TD3D12BaseView() = default;

    virtual bool IsValid() const override
    {
        return OfflineHandle != 0;
    }
};

using CD3D12RenderTargetView = TD3D12BaseView<CD3D12BaseRenderTargetView>;
using CD3D12DepthStencilView = TD3D12BaseView<CD3D12BaseDepthStencilView>;
using CD3D12UnorderedAccessView = TD3D12BaseView<CD3D12BaseUnorderedAccessView>;
using CD3D12ShaderResourceView = TD3D12BaseView<CD3D12BaseShaderResourceView>;