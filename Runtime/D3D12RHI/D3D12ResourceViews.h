#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Resource.h"
#include "D3D12RefCounted.h"

#include "RHI/RHIResourceViews.h"

class FD3D12OfflineDescriptorHeap;

typedef TSharedRef<class FD3D12ConstantBufferView>  FD3D12ConstantBufferViewRef;
typedef TSharedRef<class FD3D12ShaderResourceView>  FD3D12ShaderResourceViewRef;
typedef TSharedRef<class FD3D12UnorderedAccessView> FD3D12UnorderedAccessViewRef;
typedef TSharedRef<class FD3D12RenderTargetView>    FD3D12RenderTargetViewRef;
typedef TSharedRef<class FD3D12DepthStencilView>    FD3D12DepthStencilViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12View

class FD3D12View : public FD3D12DeviceChild
{
public:
    FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    virtual ~FD3D12View();

    bool AllocateHandle();
    void InvalidateAndFreeHandle();

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const { return OfflineHandle; }

    FORCEINLINE const FD3D12Resource* GetD3D12Resource() const { return Resource.Get(); }

protected:
    FD3D12ResourceRef            Resource;

    FD3D12OfflineDescriptorHeap* Heap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE  OfflineHandle;
    uint32                       OfflineHeapIndex = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ConstantBufferView

class FD3D12ConstantBufferView : public FD3D12View
{
public:
    FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    ~FD3D12ConstantBufferView() = default;

    bool CreateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ShaderResourceView

class FD3D12ShaderResourceView : public FRHIShaderResourceView, public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12ShaderResourceView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap, FRHIResource* InResource);
    ~FD3D12ShaderResourceView() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:
    bool CreateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12UnorderedAccessView

class FD3D12UnorderedAccessView : public FRHIUnorderedAccessView, public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12UnorderedAccessView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap, FRHIResource* InResource);
    ~FD3D12UnorderedAccessView() = default;
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:
    bool CreateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const { return Desc; }

    FORCEINLINE const FD3D12Resource* GetD3D12CounterResource() const { return CounterResource.Get(); }

private:
    FD3D12ResourceRef                CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RenderTargetView

class FD3D12RenderTargetView : public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    ~FD3D12RenderTargetView() = default;

    bool CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12DepthStencilView

class FD3D12DepthStencilView : public FD3D12View, public FD3D12RefCounted
{
public:
    FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap);
    ~FD3D12DepthStencilView() = default;

    bool CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const { return Desc; }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};
