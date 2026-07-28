#pragma once
#include "Core/RefCountedBase.h"
#include "RHI/RHIResources.h"
#include "RHI/RHITypes.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Resource.h"

class FD3D12OfflineDescriptorHeap;
class FD3D12SwapChainRHI;

typedef TSharedRef<class FD3D12ConstantBufferView>     FD3D12ConstantBufferViewRef;
typedef TSharedRef<class FD3D12ShaderResourceViewRHI>  FD3D12ShaderResourceViewRHIRef;
typedef TSharedRef<class FD3D12UnorderedAccessViewRHI> FD3D12UnorderedAccessViewRHIRef;
typedef TSharedRef<class FD3D12RenderTargetViewRHI>    FD3D12RenderTargetViewRHIRef;
typedef TSharedRef<class FD3D12DepthStencilViewRHI>    FD3D12DepthStencilViewRHIRef;

class FD3D12View : public FD3D12DeviceChild, public ID3D12ResourceRelocationListener
{
public:
    FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12View();

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool AllocateHandle();
    void InvalidateAndFreeHandle();

    void RegisterWithResource(FD3D12ResourceBase* InOwner);
    void UnregisterFromResource();

    NODISCARD FORCEINLINE bool IsValid() const
    {
        return Descriptor;
    }

    NODISCARD FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
    {
        return Descriptor.Handle;
    }

    NODISCARD FORCEINLINE const FD3D12Resource* GetViewResource() const 
    { 
        return ViewResource.Get(); 
    }

    NODISCARD FORCEINLINE FD3D12Resource* GetViewResource()
    {
        return ViewResource.Get();
    }

    NODISCARD FORCEINLINE FD3D12ResidencyHandle* GetResourceResidencyHandle() const
    {
        FD3D12Resource* Resource = ViewResource.Get();
        return Resource ? Resource->GetResidencyHandle() : nullptr;
    }

    NODISCARD FORCEINLINE void ReleaseViewResource()
    {
        ViewResource = nullptr;
    }

    NODISCARD FORCEINLINE uint32 GetDescriptorVersion() const
    {
        return DescriptorVersion;
    }

    NODISCARD FORCEINLINE bool IsRegisteredWithResource(const FD3D12ResourceBase* Resource) const
    {
        return OwnerResource == Resource;
    }

    NODISCARD FORCEINLINE FRHIDescriptorHandle GetCachedBindlessHandle() const
    {
        return BindlessHandle;
    }

protected:
    FRHIDescriptorHandle EnsureBindlessHandle(EDescriptorType InType) const;
    void IncrementDescriptorVersion();

    FD3D12ResourceRef            ViewResource;
    FD3D12OfflineDescriptorHeap& OfflineHeap;
    FD3D12OfflineDescriptor      Descriptor;
    FD3D12ResourceBase*          OwnerResource;
    uint32                       DescriptorVersion;
    mutable FRHIDescriptorHandle BindlessHandle;
};

class FD3D12ConstantBufferView : public FD3D12View, public FRefCountedBase
{
public:
    FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap);
    virtual ~FD3D12ConstantBufferView() = default;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool Initialize(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);
    bool UpdateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

    NODISCARD FRHIDescriptorHandle GetBindlessHandle() const
    {
        return EnsureBindlessHandle(EDescriptorType::ConstantBuffer);
    }

    NODISCARD FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

private:
    D3D12_CONSTANT_BUFFER_VIEW_DESC D3D12Desc;
};

class FD3D12ShaderResourceViewRHI : public FRHIShaderResourceView, public FD3D12View
{
public:
    FD3D12ShaderResourceViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc);
    virtual ~FD3D12ShaderResourceViewRHI() = default;

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool Initialize(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);
    bool UpdateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

    NODISCARD FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC D3D12Desc;
};

class FD3D12UnorderedAccessViewBase : public FRHIUnorderedAccessView
{
protected:
    FD3D12UnorderedAccessViewBase(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
        : FRHIUnorderedAccessView(InResource, InDesc)
    {
    }

    virtual ~FD3D12UnorderedAccessViewBase() = default;

public:
    virtual class FD3D12UnorderedAccessViewRHI* GetUnorderedAccessViewInterface() const = 0;
};

class FD3D12UnorderedAccessViewRHI : public FD3D12UnorderedAccessViewBase, public FD3D12View
{
public:
    FD3D12UnorderedAccessViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc);
    virtual ~FD3D12UnorderedAccessViewRHI() = default;
 
    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // FD3D12UnorderedAccessViewBase Interface
    virtual FD3D12UnorderedAccessViewRHI* GetUnorderedAccessViewInterface() const override;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool Initialize(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);
    bool UpdateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

    NODISCARD FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

    NODISCARD FORCEINLINE const FD3D12Resource* GetCounterResource() const
    { 
        return CounterResource.Get(); 
    }

private:
    FD3D12ResourceRef                CounterResource;
    D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12Desc;
};

class FD3D12RenderTargetViewBase : public FRHIRenderTargetView
{
protected:
    FD3D12RenderTargetViewBase(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
        : FRHIRenderTargetView(InResource, InDesc)
    {
    }

    virtual ~FD3D12RenderTargetViewBase() = default;

public:
    virtual FD3D12RenderTargetViewRHI* GetRenderTargetViewInterface() const = 0;
};

class FD3D12RenderTargetViewRHI : public FD3D12RenderTargetViewBase, public FD3D12View
{
public:
    FD3D12RenderTargetViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc);
    virtual ~FD3D12RenderTargetViewRHI() = default;

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    // FD3D12RenderTargetViewBase Interface
    virtual FD3D12RenderTargetViewRHI* GetRenderTargetViewInterface() const override;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool Initialize(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);
    bool UpdateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

    NODISCARD FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

private:
    D3D12_RENDER_TARGET_VIEW_DESC D3D12Desc;
};

class FD3D12DepthStencilViewRHI : public FRHIDepthStencilView, public FD3D12View
{
public:
    FD3D12DepthStencilViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc);
    virtual ~FD3D12DepthStencilViewRHI() = default;

    // FRHIDepthStencilView Interface
    virtual void* GetRHINativeHandle() const override final;

    // ID3D12ResourceRelocationListener Interface
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) override;

    bool Initialize(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);
    bool UpdateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

    NODISCARD FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

    NODISCARD FORCEINLINE bool HasStencilFormat() const
    {
        return bHasStencil;
    }

    NODISCARD FORCEINLINE bool IsReadOnly() const
    {
        return IsDepthReadOnly() && (!HasStencilFormat() || IsStencilReadOnly());
    }

    NODISCARD FORCEINLINE bool IsDepthReadOnly() const
    {
        return (D3D12Desc.Flags & D3D12_DSV_FLAG_READ_ONLY_DEPTH) != 0;
    }

    NODISCARD FORCEINLINE bool IsStencilReadOnly() const
    {
        return (D3D12Desc.Flags & D3D12_DSV_FLAG_READ_ONLY_STENCIL) != 0;
    }

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC D3D12Desc;
    bool                          bHasStencil;
};
