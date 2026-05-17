#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12ResidencyManager.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12Texture.h"

FD3D12View::FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12DeviceChild(InDevice)
    , ViewResource(nullptr)
    , OfflineHeap(InOfflineHeap)
    , Descriptor()
    , OwnerResource(nullptr)
    , DescriptorVersion(0)
    , BindlessHandle()
{
}

FD3D12View::~FD3D12View()
{
    UnregisterFromResource();
    InvalidateAndFreeHandle();

    if (BindlessHandle.IsValid())
    {
        if (FD3D12BindlessDescriptorHeap* Heap = GetDevice()->GetResourceBindlessHeap())
        {
            Heap->Free(BindlessHandle);
        }
        
        BindlessHandle = FRHIDescriptorHandle();
    }
}

void FD3D12View::RegisterWithResource(FD3D12ResourceBase* InOwner)
{
    if (InOwner == OwnerResource)
    {
        return;
    }

    UnregisterFromResource();

    OwnerResource = InOwner;
    if (OwnerResource)
    {
        OwnerResource->AddResourceRelocatedListener(this);
    }
}

void FD3D12View::UnregisterFromResource()
{
    if (OwnerResource)
    {
        OwnerResource->RemoveResourceRelocatedListener(this);
        OwnerResource = nullptr;
    }
}

void FD3D12View::OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    CHECK(RelocatedResource == OwnerResource);

    if (!NewResourceStorage)
    {
        OwnerResource = nullptr;
    }
}

bool FD3D12View::AllocateHandle()
{
    Descriptor = OfflineHeap.Allocate();
    return Descriptor;
}

void FD3D12View::InvalidateAndFreeHandle()
{
	if (Descriptor)
	{
	    OfflineHeap.Free(Descriptor);
        Descriptor = {};
    }
}

FRHIDescriptorHandle FD3D12View::EnsureBindlessHandle(EDescriptorType InType) const
{
    if (BindlessHandle.IsValid())
    {
        return BindlessHandle;
    }

    FD3D12BindlessDescriptorHeap* BindlessHeap = GetDevice()->GetResourceBindlessHeap();
    if (!BindlessHeap)
    {
        return FRHIDescriptorHandle();
    }

    if (!Descriptor)
    {
        return FRHIDescriptorHandle();
    }

    BindlessHandle = BindlessHeap->Allocate(InType);
    if (!BindlessHandle.IsValid())
    {
        return FRHIDescriptorHandle();
    }

    BindlessHeap->EnqueueWrite(BindlessHandle, Descriptor.Handle);

    if (FD3D12Resource* Resource = ViewResource.Get())
    {
        Resource->EndResidencyTracking();
    }

    return BindlessHandle;
}

void FD3D12View::IncrementDescriptorVersion()
{
    ++DescriptorVersion;

    if (BindlessHandle.IsValid())
    {
        if (FD3D12BindlessDescriptorHeap* BindlessHeap = GetDevice()->GetResourceBindlessHeap())
        {
            BindlessHeap->EnqueueWrite(BindlessHandle, Descriptor.Handle);
        }
    }
}

FD3D12ConstantBufferView::FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12View(InDevice, InOfflineHeap)
    , D3D12Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FD3D12ConstantBufferView::OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC NewDesc = D3D12Desc;
        NewDesc.BufferLocation = NewResourceStorage->GetGPUVirtualAddress();

        UpdateView(NewResourceStorage->GetResource(), NewDesc);
    }
}

bool FD3D12ConstantBufferView::Initialize(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    if (!AllocateHandle())
    {
        return false;
    }

    return UpdateView(InResource, InDesc);
}

bool FD3D12ConstantBufferView::UpdateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12ConstantBufferView] Invalid Descriptor");
        return false;
    }

    D3D12Desc    = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    GetDevice()->GetD3D12Device()->CreateConstantBufferView(&D3D12Desc, GetOfflineHandle());

    IncrementDescriptorVersion();
    return true;
}

FD3D12ShaderResourceViewRHI::FD3D12ShaderResourceViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FD3D12View(InDevice, InOfflineHeap)
    , D3D12Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void* FD3D12ShaderResourceViewRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(static_cast<UPTR_INT>(GetOfflineHandle().ptr));
}

FRHIDescriptorHandle FD3D12ShaderResourceViewRHI::GetBindlessHandle() const
{
    return EnsureBindlessHandle(EDescriptorType::ShaderResource);
}

void FD3D12ShaderResourceViewRHI::OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        UpdateView(NewResourceStorage->GetResource(), D3D12Desc);
    }
}

bool FD3D12ShaderResourceViewRHI::Initialize(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    if (!AllocateHandle())
    {
        return false;
    }

    return UpdateView(InResource, InDesc);
}

bool FD3D12ShaderResourceViewRHI::UpdateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12ShaderResourceViewRHI] Invalid Descriptor");
        return false;
    }

    D3D12Desc    = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateShaderResourceView(D3DResource, &D3D12Desc, GetOfflineHandle());

    IncrementDescriptorVersion();
    return true;
}

FD3D12UnorderedAccessViewRHI::FD3D12UnorderedAccessViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FD3D12UnorderedAccessViewBase(InResource, InRHIDesc)
    , FD3D12View(InDevice, InOfflineHeap)
    , CounterResource(nullptr)
    , D3D12Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void* FD3D12UnorderedAccessViewRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(static_cast<UPTR_INT>(GetOfflineHandle().ptr));
}

FRHIDescriptorHandle FD3D12UnorderedAccessViewRHI::GetBindlessHandle() const
{
    return EnsureBindlessHandle(EDescriptorType::UnorderedAccess);
}

FD3D12UnorderedAccessViewRHI* FD3D12UnorderedAccessViewRHI::GetUnorderedAccessViewInterface() const
{
    return const_cast<FD3D12UnorderedAccessViewRHI*>(this);
}

void FD3D12UnorderedAccessViewRHI::OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        UpdateView(CounterResource.Get(), NewResourceStorage->GetResource(), D3D12Desc);
    }
}

bool FD3D12UnorderedAccessViewRHI::Initialize(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    if (!AllocateHandle())
    {
        return false;
    }

    return UpdateView(InCounterResource, InResource, InDesc);
}

bool FD3D12UnorderedAccessViewRHI::UpdateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12UnorderedAccessViewRHI] Invalid Descriptor");
        return false;
    }

    D3D12Desc       = InDesc;
    CounterResource = MakeSharedRef<FD3D12Resource>(InCounterResource);
    ViewResource    = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* D3DCounterResource = nullptr;
    if (CounterResource)
    {
        D3DCounterResource = CounterResource->GetD3D12Resource();
    }

    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(D3DResource, D3DCounterResource, &D3D12Desc, GetOfflineHandle());

    IncrementDescriptorVersion();
    return true;
}

FD3D12RenderTargetViewRHI::FD3D12RenderTargetViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc)
    : FD3D12RenderTargetViewBase(InResource, InRHIDesc)
    , FD3D12View(InDevice, InOfflineHeap)
    , D3D12Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void* FD3D12RenderTargetViewRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(static_cast<UPTR_INT>(GetOfflineHandle().ptr));
}

FD3D12RenderTargetViewRHI* FD3D12RenderTargetViewRHI::GetRenderTargetViewInterface() const
{
    return const_cast<FD3D12RenderTargetViewRHI*>(this);
}

void FD3D12RenderTargetViewRHI::OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        UpdateView(NewResourceStorage->GetResource(), D3D12Desc);
    }
}

bool FD3D12RenderTargetViewRHI::Initialize(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    if (!AllocateHandle())
    {
        return false;
    }

    return UpdateView(InResource, InDesc);
}

bool FD3D12RenderTargetViewRHI::UpdateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12RenderTargetViewRHI] Invalid Descriptor");
        return false;
    }

    D3D12Desc    = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateRenderTargetView(D3DResource, &D3D12Desc, GetOfflineHandle());

    IncrementDescriptorVersion();
    return true;
}

FD3D12DepthStencilViewRHI::FD3D12DepthStencilViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc)
    : FRHIDepthStencilView(InResource, InRHIDesc)
    , FD3D12View(InDevice, InOfflineHeap)
    , D3D12Desc()
    , bHasStencil(false)
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void* FD3D12DepthStencilViewRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(static_cast<UPTR_INT>(GetOfflineHandle().ptr));
}

void FD3D12DepthStencilViewRHI::OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        UpdateView(NewResourceStorage->GetResource(), D3D12Desc);
    }
}

bool FD3D12DepthStencilViewRHI::Initialize(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    if (!AllocateHandle())
    {
        return false;
    }
    
    return UpdateView(InResource, InDesc);
}

bool FD3D12DepthStencilViewRHI::UpdateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12DepthStencilViewRHI] Invalid Descriptor");
        return false;
    }

    D3D12Desc    = InDesc;
    bHasStencil  = IsStencilFormat(InDesc.Format);
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateDepthStencilView(D3DResource, &D3D12Desc, GetOfflineHandle());

    IncrementDescriptorVersion();
    return true;
}
