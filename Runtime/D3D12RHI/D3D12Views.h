#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"

#include "RHI/RHIResourceViews.h"

class CD3D12Device;
class CD3D12OfflineDescriptorHeap;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12View

class CD3D12View : public CD3D12DeviceChild
{
public:
    CD3D12View(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    virtual ~CD3D12View();

    bool AllocateHandle();

    void InvalidateAndFreeHandle();

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    FORCEINLINE const CD3D12Resource* GetD3D12Resource() const { return Resource.Get(); }

protected:
    TSharedRef<CD3D12Resource> Resource;
 
    CD3D12OfflineDescriptorHeap* Heap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    uint32                       OfflineHeapIndex = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ConstantBufferView

class CD3D12ConstantBufferView : public CD3D12View
{
public:
    CD3D12ConstantBufferView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12ConstantBufferView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ShaderResourceView

class CD3D12ShaderResourceView : public CRHIShaderResourceView, public CD3D12View
{
public:
    CD3D12ShaderResourceView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, CRHIResource* InResource);
    ~CD3D12ShaderResourceView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12UnorderedAccessView

class CD3D12UnorderedAccessView : public CRHIUnorderedAccessView, public CD3D12View
{
public:
    CD3D12UnorderedAccessView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, CRHIResource* InResource);
    ~CD3D12UnorderedAccessView() = default;

    bool CreateView(CD3D12Resource* InCounterResource, CD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const { return Desc; }

    FORCEINLINE const CD3D12Resource* GetD3D12CounterResource() const { return CounterResource.Get(); }

private:
    TSharedRef<CD3D12Resource>       CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RenderTargetView

class CD3D12RenderTargetView : public CRHIRenderTargetView, public CD3D12View
{
public:
    CD3D12RenderTargetView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12RenderTargetView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12DepthStencilView

class CD3D12DepthStencilView : public CRHIDepthStencilView, public CD3D12View
{
public:
    CD3D12DepthStencilView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap);
    ~CD3D12DepthStencilView() = default;

    bool CreateView(CD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};
