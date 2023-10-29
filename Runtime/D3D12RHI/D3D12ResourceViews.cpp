#include "D3D12Device.h"
#include "D3D12Descriptors.h"
#include "D3D12ResourceViews.h"

FD3D12View::FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , Heap(InHeap)
    , Descriptor()
{
    CHECK(Heap != nullptr);
}

FD3D12View::~FD3D12View()
{
    InvalidateAndFreeHandle();
}

bool FD3D12View::AllocateHandle()
{
    Descriptor = Heap->Allocate();
    return Descriptor;
}

void FD3D12View::InvalidateAndFreeHandle()
{
    Heap->Free(Descriptor);
}


FD3D12ConstantBufferView::FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12ConstantBufferView::CreateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR("[FD3D12ConstantBufferView] Invalid Descriptor");
        return false;
    }

    Resource = MakeSharedRef<FD3D12Resource>(InResource);
    Desc = InDesc;
    GetDevice()->GetD3D12Device()->CreateConstantBufferView(&Desc, GetOfflineHandle());
    return true;
}


FD3D12ShaderResourceView::FD3D12ShaderResourceView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12ShaderResourceView::CreateView(FD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR("[FD3D12ShaderResourceView] Invalid Descriptor");
        return false;
    }

    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);
    Desc = InDesc;

    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateShaderResourceView(NativeResource, &Desc, GetOfflineHandle());
    return true;
}


FD3D12UnorderedAccessView::FD3D12UnorderedAccessView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FD3D12View(InDevice, InHeap)
    , Desc()
    , CounterResource(nullptr)
{
}

bool FD3D12UnorderedAccessView::CreateView(FD3D12Resource* InCounterResource, FD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR("[FD3D12UnorderedAccessView] Invalid Descriptor");
        return false;
    }

    Desc            = InDesc;
    CounterResource = InCounterResource;
    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* NativeCounterResource = nullptr;
    if (CounterResource)
    {
        NativeCounterResource = CounterResource->GetD3D12Resource();
    }

    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(NativeResource, NativeCounterResource, &Desc, GetOfflineHandle());
    return true;
}


FD3D12RenderTargetView::FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12RenderTargetView::CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR("[FD3D12RenderTargetView] Invalid Descriptor");
        return false;
    }

    Desc = InDesc;
    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateRenderTargetView(NativeResource, &Desc, GetOfflineHandle());
    return true;
}


FD3D12DepthStencilView::FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12DepthStencilView::CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    if (!Descriptor)
    {
        D3D12_ERROR("[FD3D12DepthStencilView] Invalid Descriptor");
        return false;
    }

    Desc = InDesc;
    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);
    
    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateDepthStencilView(NativeResource, &Desc, GetOfflineHandle());
    return true;
}
