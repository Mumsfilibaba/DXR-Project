#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12ResourceViews.h"

FD3D12View::FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12DeviceChild(InDevice)
    , ViewResource(nullptr)
    , OfflineHeap(InOfflineHeap)
    , Descriptor()
    , OwnerResource(nullptr)
    , DescriptorVersion(0)
{
}

FD3D12View::~FD3D12View()
{
    UnregisterFromResource();
    InvalidateAndFreeHandle();
}

void FD3D12View::RegisterWithResource(FD3D12GenericResource* InOwner)
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

void FD3D12View::OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
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

FD3D12ConstantBufferView::FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12View(InDevice, InOfflineHeap)
    , Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FD3D12ConstantBufferView::OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC NewDesc = Desc;
        NewDesc.BufferLocation = NewResourceStorage->GetGPUVirtualAddress();

        CreateView(NewResourceStorage->GetResource(), NewDesc);
    }
}

bool FD3D12ConstantBufferView::CreateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12ConstantBufferView] Invalid Descriptor");
        return false;
    }

    Desc         = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    GetDevice()->GetD3D12Device()->CreateConstantBufferView(&Desc, GetOfflineHandle());
    IncrementDescriptorVersion();
    return true;
}

FD3D12ShaderResourceViewRHI::FD3D12ShaderResourceViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FD3D12View(InDevice, InOfflineHeap)
    , Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FD3D12ShaderResourceViewRHI::OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        CreateView(NewResourceStorage->GetResource(), Desc);
    }
}

bool FD3D12ShaderResourceViewRHI::CreateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12ShaderResourceViewRHI] Invalid Descriptor");
        return false;
    }

    Desc         = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateShaderResourceView(D3DResource, &Desc, GetOfflineHandle());
    IncrementDescriptorVersion();
    return true;
}

FD3D12UnorderedAccessViewRHI::FD3D12UnorderedAccessViewRHI(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FD3D12View(InDevice, InOfflineHeap)
    , Desc()
    , CounterResource(nullptr)
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FD3D12UnorderedAccessViewRHI::OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        CreateView(CounterResource.Get(), NewResourceStorage->GetResource(), Desc);
    }
}

bool FD3D12UnorderedAccessViewRHI::CreateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12UnorderedAccessViewRHI] Invalid Descriptor");
        return false;
    }

    Desc            = InDesc;
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

    GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(D3DResource, D3DCounterResource, &Desc, GetOfflineHandle());
    IncrementDescriptorVersion();
    return true;
}

FD3D12RenderTargetView::FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12View(InDevice, InOfflineHeap)
    , Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void FD3D12RenderTargetView::OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        CreateView(NewResourceStorage->GetResource(), Desc);
    }
}

bool FD3D12RenderTargetView::CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12RenderTargetView] Invalid Descriptor");
        return false;
    }

    Desc         = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateRenderTargetView(D3DResource, &Desc, GetOfflineHandle());
    IncrementDescriptorVersion();
    return true;
}

FD3D12DepthStencilView::FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12View(InDevice, InOfflineHeap)
    , Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void FD3D12DepthStencilView::OnResourceRelocated(FD3D12GenericResource* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage)
{
    FD3D12View::OnResourceRelocated(RelocatedResource, NewResourceStorage);

    if (NewResourceStorage)
    {
        CreateView(NewResourceStorage->GetResource(), Desc);
    }
}

bool FD3D12DepthStencilView::CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12DepthStencilView] Invalid Descriptor");
        return false;
    }

    Desc         = InDesc;
    ViewResource = MakeSharedRef<FD3D12Resource>(InResource);
    
    ID3D12Resource* D3DResource = nullptr;
    if (ViewResource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0);
        D3DResource = ViewResource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateDepthStencilView(D3DResource, &Desc, GetOfflineHandle());
    IncrementDescriptorVersion();
    return true;
}
