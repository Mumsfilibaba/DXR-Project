#include "D3D12Device.h"
#include "D3D12Descriptors.h"
#include "D3D12ResourceViews.h"

FD3D12View::FD3D12View(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , Heap(InHeap)
    , OfflineHandle({ 0 })
{
    CHECK(Heap != nullptr);
}

FD3D12View::~FD3D12View()
{
    InvalidateAndFreeHandle();
}

bool FD3D12View::AllocateHandle()
{
    OfflineHandle = Heap->Allocate(OfflineHeapIndex);
    return OfflineHandle != 0;
}

void FD3D12View::InvalidateAndFreeHandle()
{
    Heap->Free(OfflineHandle, OfflineHeapIndex);
    OfflineHeapIndex = 0;
    OfflineHandle    = { 0 };
}


FD3D12ConstantBufferView::FD3D12ConstantBufferView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12ConstantBufferView::CreateView(FD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    CHECK(OfflineHandle != 0);

    Resource = MakeSharedRef<FD3D12Resource>(InResource);
    Desc = InDesc;
    GetDevice()->GetD3D12Device()->CreateConstantBufferView(&Desc, OfflineHandle);
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
    CHECK(OfflineHandle != 0);

    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);
    Desc = InDesc;

    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateShaderResourceView(NativeResource, &Desc, OfflineHandle);
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
    CHECK(OfflineHandle != 0);

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

    GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(NativeResource, NativeCounterResource, &Desc, OfflineHandle);
    return true;
}


FD3D12RenderTargetView::FD3D12RenderTargetView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12RenderTargetView::CreateView(FD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    CHECK(OfflineHandle != 0);

    Desc = InDesc;
    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);

    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateRenderTargetView(NativeResource, &Desc, OfflineHandle);
    return true;
}


FD3D12DepthStencilView::FD3D12DepthStencilView(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InHeap)
    : FD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool FD3D12DepthStencilView::CreateView(FD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    CHECK(OfflineHandle != 0);

    Desc = InDesc;
    FD3D12View::Resource = MakeSharedRef<FD3D12Resource>(InResource);
    
    ID3D12Resource* NativeResource = nullptr;
    if (FD3D12View::Resource)
    {
        CHECK((InResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0);
        NativeResource = FD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->GetD3D12Device()->CreateDepthStencilView(NativeResource, &Desc, OfflineHandle);
    return true;
}
