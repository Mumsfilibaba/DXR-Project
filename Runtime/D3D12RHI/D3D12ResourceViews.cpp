#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12ResourceViews.h"

FD3D12View::FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap)
    : FD3D12DeviceChild(InDevice)
    , ViewResource(nullptr)
    , OfflineHeap(InOfflineHeap)
    , Descriptor()
{
}

FD3D12View::~FD3D12View()
{
    UnregisterFromResource();
    InvalidateAndFreeHandle();
}

void FD3D12View::RegisterWithResource(FD3D12BaseResource* InOwner)
{
    if (InOwner == OwnerResource)
    {
        return;
    }

    UnregisterFromResource();

    OwnerResource = InOwner;
    if (OwnerResource)
    {
        OwnerResource->AddListener(this);
    }
}

void FD3D12View::UnregisterFromResource()
{
    if (OwnerResource)
    {
        OwnerResource->RemoveListener(this);
        OwnerResource = nullptr;
    }
}

void FD3D12View::OnOwnerReleased()
{
    OwnerResource = nullptr;
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

void FD3D12ConstantBufferView::OnRelocation(FD3D12BaseResource* InResource)
{
    FD3D12ResourceStorage& Storage = InResource->GetResourceStorage();

    D3D12_CONSTANT_BUFFER_VIEW_DESC NewDesc = Desc;
    NewDesc.BufferLocation = Storage.GetGpuVirtualAddress();

    CreateView(Storage.GetResource(), NewDesc);
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

FD3D12ShaderResourceView::FD3D12ShaderResourceView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FD3D12View(InDevice, InOfflineHeap)
    , Desc()
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FD3D12ShaderResourceView::OnRelocation(FD3D12BaseResource* InResource)
{
    CreateView(InResource->GetResource(), Desc);
}

bool FD3D12ShaderResourceView::CreateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12ShaderResourceView] Invalid Descriptor");
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

FD3D12UnorderedAccessView::FD3D12UnorderedAccessView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap& InOfflineHeap, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FD3D12View(InDevice, InOfflineHeap)
    , Desc()
    , CounterResource(nullptr)
{
    CHECK(InOfflineHeap.GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FD3D12UnorderedAccessView::OnRelocation(FD3D12BaseResource* InResource)
{
    CreateView(CounterResource.Get(), InResource->GetResource(), Desc);
}

bool FD3D12UnorderedAccessView::CreateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR_CRITICAL("[FD3D12UnorderedAccessView] Invalid Descriptor");
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

void FD3D12RenderTargetView::OnRelocation(FD3D12BaseResource* InResource)
{
    CreateView(InResource->GetResource(), Desc);
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

void FD3D12DepthStencilView::OnRelocation(FD3D12BaseResource* InResource)
{
    CreateView(InResource->GetResource(), Desc);
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
