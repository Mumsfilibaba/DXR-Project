#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"
#include "D3D12RefCounted.h"
#include "RHI/RHIResources.h"

class FD3D12OfflineDescriptorHeap;

typedef TSharedRef<class FD3D12ConstantBufferView>  FD3D12ConstantBufferViewRef;
typedef TSharedRef<class FD3D12ShaderResourceView>  FD3D12ShaderResourceViewRef;
typedef TSharedRef<class FD3D12UnorderedAccessView> FD3D12UnorderedAccessViewRef;
typedef TSharedRef<class FD3D12RenderTargetView>    FD3D12RenderTargetViewRef;
typedef TSharedRef<class FD3D12DepthStencilView>    FD3D12DepthStencilViewRef;


class FD3D12View : public FD3D12DeviceChild
{
public:
    FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    virtual ~FD3D12View();

    bool AllocateHandle();
    void InvalidateAndFreeHandle();

    D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return Descriptor.Handle;
    }

    const FD3D12Resource* GetD3D12Resource() const 
    { 
        return Resource.Get(); 
    }

protected:
    FD3D12ResourceRef            Resource;
    FD3D12OfflineDescriptorHeap* Heap;
    FD3D12OfflineDescriptor      Descriptor;
};


class FD3D12ConstantBufferView : public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);

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
    FD3D12ShaderResourceView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap, FRHIResource* InResource);
    virtual ~FD3D12ShaderResourceView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

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
    FD3D12UnorderedAccessView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap, FRHIResource* InResource);
    virtual ~FD3D12UnorderedAccessView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    bool CreateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const
    { 
        return Desc;
    }

    const FD3D12Resource* GetD3D12CounterResource() const
    { 
        return CounterResource.Get(); 
    }

private:
    FD3D12ResourceRef                CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};


class FD3D12RenderTargetView : public FD3D12View
{
public:
    FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    virtual ~FD3D12RenderTargetView() = default;

    bool CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const 
    {
        return Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};


class FD3D12DepthStencilView : public FD3D12View
{
public:
    FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    virtual ~FD3D12DepthStencilView() = default;

    bool CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const 
    { 
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};
