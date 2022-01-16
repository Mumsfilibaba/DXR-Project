#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"

#include "RHI/RHIResourceViews.h"

class CD3D12Device;
class CD3D12OfflineDescriptorHeap;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CD3D12View : public CD3D12DeviceChild
{
public:
    CD3D12View(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    virtual ~CD3D12View();

    /* Allocates a descriptor-handle from the offline heap */
    bool AllocateHandle();

    /* Sets the handle to zero and frees the current handle back to the heap */
    void InvalidateAndFreeHandle();

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return OfflineHandle;
    }

    FORCEINLINE const CD3D12Resource* GetResource() const
    {
        return Resource.Get();
    }

protected:

    // The resource that the view is belonging to 
    TSharedRef<CD3D12Resource> Resource;

    // Heap that this view belongs to 
    CD3D12OfflineDescriptorHeap* Heap = nullptr;

    // Offline handle, allocated from the heap
    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;

    // Index to the offline handle in the offline heap
    uint32 OfflineHeapIndex = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CD3D12RHIConstantBufferView : public CD3D12View
{
public:

    CD3D12RHIConstantBufferView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12RHIConstantBufferView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CD3D12RHIBaseShaderResourceView : public CRHIShaderResourceView, public CD3D12View
{
public:

    CD3D12RHIBaseShaderResourceView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12RHIBaseShaderResourceView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CD3D12RHIBaseUnorderedAccessView : public CRHIUnorderedAccessView, public CD3D12View
{
public:

    CD3D12RHIBaseUnorderedAccessView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12RHIBaseUnorderedAccessView() = default;

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

class CD3D12RHIBaseRenderTargetView : public CRHIRenderTargetView, public CD3D12View
{
public:

    CD3D12RHIBaseRenderTargetView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12RHIBaseRenderTargetView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CD3D12RHIBaseDepthStencilView : public CRHIDepthStencilView, public CD3D12View
{
public:

    CD3D12RHIBaseDepthStencilView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12RHIBaseDepthStencilView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

template<typename BaseViewType>
class TD3D12RHIBaseView : public BaseViewType
{
public:

    TD3D12RHIBaseView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
        : BaseViewType(InDevice, InHeap)
    {
    }

    ~TD3D12RHIBaseView() = default;

    virtual bool IsValid() const override
    {
        return (OfflineHandle != 0);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

using CD3D12RenderTargetView = TD3D12RHIBaseView<CD3D12RHIBaseRenderTargetView>;
using CD3D12DepthStencilView = TD3D12RHIBaseView<CD3D12RHIBaseDepthStencilView>;
using CD3D12RHIUnorderedAccessView = TD3D12RHIBaseView<CD3D12RHIBaseUnorderedAccessView>;
using CD3D12RHIShaderResourceView = TD3D12RHIBaseView<CD3D12RHIBaseShaderResourceView>;