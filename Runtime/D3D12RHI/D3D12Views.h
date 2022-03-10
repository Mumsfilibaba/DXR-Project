#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"

#include "RHI/RHIResourceViews.h"

class CD3D12OfflineDescriptorHeap;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Views

template<typename BaseViewType>
class TD3D12BaseView;

using CD3D12RenderTargetView    = TD3D12BaseView<class CD3D12BaseRenderTargetView>;
using CD3D12DepthStencilView    = TD3D12BaseView<class CD3D12BaseDepthStencilView>;
using CD3D12UnorderedAccessView = TD3D12BaseView<class CD3D12BaseUnorderedAccessView>;
using CD3D12ShaderResourceView  = TD3D12BaseView<class CD3D12BaseShaderResourceView>;

typedef TSharedRef<class CD3D12ConstantBufferView> CD3D12ConstantBufferViewRef;
typedef TSharedRef<CD3D12RenderTargetView>         CD3D12RenderTargetViewRef;
typedef TSharedRef<CD3D12DepthStencilView>         CD3D12DepthStencilViewRef;
typedef TSharedRef<CD3D12UnorderedAccessView>      CD3D12UnorderedAccessViewRef;
typedef TSharedRef<CD3D12ShaderResourceView>       CD3D12ShaderResourceViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12View

class CD3D12View : public CD3D12DeviceObject
{
public:
    CD3D12View(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    virtual ~CD3D12View();

    bool AllocateHandle();
    void InvalidateAndFreeHandle();

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return OfflineHandle;
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetOnlineHandle() const
    {
        return OnlineHandle;
    }

    FORCEINLINE const CD3D12Resource* GetResource() const
    {
        return Resource.Get();
    }

protected:
    CD3D12ResourceRef Resource;
 
    CD3D12OfflineDescriptorHeap* Heap             = nullptr;
    uint32                       OfflineHeapIndex = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE  OnlineHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ConstantBufferView

class CD3D12ConstantBufferView : public CD3D12View, public CRefCounted
{
public:

    CD3D12ConstantBufferView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12ConstantBufferView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseShaderResourceView

class CD3D12BaseShaderResourceView : public CRHIShaderResourceView, public CD3D12View
{
public:

    CD3D12BaseShaderResourceView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12BaseShaderResourceView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseUnorderedAccessView

class CD3D12BaseUnorderedAccessView : public CRHIUnorderedAccessView, public CD3D12View
{
public:

    CD3D12BaseUnorderedAccessView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12BaseUnorderedAccessView() = default;

    bool CreateView(CD3D12Resource* InCounterResource, CD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

    FORCEINLINE const CD3D12Resource* GetCounterResource() const
    {
        return CounterResource.Get();
    }

private:
    TSharedRef<CD3D12Resource> CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRenderTargetView

class CD3D12BaseRenderTargetView : public CRHIRenderTargetView, public CD3D12View
{
public:

    CD3D12BaseRenderTargetView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12BaseRenderTargetView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseDepthStencilView

class CD3D12BaseDepthStencilView : public CRHIDepthStencilView, public CD3D12View
{
public:

    CD3D12BaseDepthStencilView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12BaseDepthStencilView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12BaseView

template<typename BaseViewType>
class TD3D12BaseView : public BaseViewType
{
public:

    TD3D12BaseView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
        : BaseViewType(InDevice, InHeap)
    {
    }

    ~TD3D12BaseView() = default;

    virtual bool IsValid() const override
    {
        return (OfflineHandle != 0);
    }
};