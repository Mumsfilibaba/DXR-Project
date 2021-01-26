#pragma once
#include "RenderLayer/ResourceViews.h"

#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"

class D3D12Device;
class D3D12OfflineDescriptorHeap;

class D3D12View : public D3D12DeviceChild
{
public:
    D3D12View(D3D12Device* InDevice, const D3D12Resource* InResource);
    virtual ~D3D12View();

    void ResetResource();

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return OfflineHandle;
    }

    FORCEINLINE const D3D12Resource* GetResource() const
    {
        return Resource;
    }

protected:
    const D3D12Resource* Resource = nullptr;
    D3D12OfflineDescriptorHeap* Heap             = nullptr;
    UInt32                      OfflineHeapIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
};

class D3D12ConstantBufferView : public D3D12View
{
public:
    D3D12ConstantBufferView(D3D12Device* InDevice, const D3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    void CreateView(const D3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

class D3D12ShaderResourceView : public ShaderResourceView, public D3D12View
{
public:
    D3D12ShaderResourceView(D3D12Device* InDevice, const D3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    void CreateView(const D3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

class D3D12UnorderedAccessView : public UnorderedAccessView, public D3D12View
{
public:
    D3D12UnorderedAccessView(D3D12Device* InDevice, const D3D12Resource* InCounterResource, const D3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    void CreateView(const D3D12Resource* InCounterResource, const D3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    const D3D12Resource* CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

class D3D12RenderTargetView : public RenderTargetView, public D3D12View
{
public:
    D3D12RenderTargetView(D3D12Device* InDevice, const D3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    void CreateView(const D3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

class D3D12DepthStencilView : public DepthStencilView, public D3D12View
{
public:
    D3D12DepthStencilView(D3D12Device* InDevice, const D3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    void CreateView(const D3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};