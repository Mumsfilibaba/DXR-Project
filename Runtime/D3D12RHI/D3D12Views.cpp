#include "D3D12Device.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Views.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12View

CD3D12View::CD3D12View(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12DeviceObject(InDevice)
    , Resource(nullptr)
    , Heap(InHeap)
    , OfflineHandle({ 0 })
{
    Assert(Heap != nullptr);
}

CD3D12View::~CD3D12View()
{
    // NOTE: Does not follow the rest of the engine's explicit alloc/dealloc, Ok for now
    InvalidateAndFreeHandle();
}

bool CD3D12View::AllocateHandle()
{
    OfflineHandle = Heap->Allocate(OfflineHeapIndex);
    return OfflineHandle != 0;
}

void CD3D12View::InvalidateAndFreeHandle()
{
    Heap->Free(OfflineHandle, OfflineHeapIndex);
    OfflineHeapIndex = 0;
    OfflineHandle = { 0 };
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIConstantBufferView

CD3D12ConstantBufferView::CD3D12ConstantBufferView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12ConstantBufferView::CreateView(CD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    Assert(OfflineHandle != 0);

    Resource = MakeSharedRef<CD3D12Resource>(InResource);
    Desc = InDesc;
    GetDevice()->CreateConstantBufferView(&Desc, OfflineHandle);

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseShaderResourceView

CD3D12BaseShaderResourceView::CD3D12BaseShaderResourceView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12BaseShaderResourceView::CreateView(CD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    Assert(OfflineHandle != 0);

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);
    Desc = InDesc;

    ID3D12Resource* NativeResource = nullptr;
    if (CD3D12View::Resource)
    {
        NativeResource = CD3D12View::Resource->GetResource();
    }

    GetDevice()->CreateShaderResourceView(NativeResource, &Desc, OfflineHandle);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseUnorderedAccessView

CD3D12BaseUnorderedAccessView::CD3D12BaseUnorderedAccessView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
    , CounterResource(nullptr)
{
}

bool CD3D12BaseUnorderedAccessView::CreateView(CD3D12Resource* InCounterResource, CD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    Assert(OfflineHandle != 0);

    Desc = InDesc;
    CounterResource = InCounterResource;
    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);

    ID3D12Resource* NativeCounterResource = nullptr;
    if (CounterResource)
    {
        NativeCounterResource = CounterResource->GetResource();
    }

    ID3D12Resource* NativeResource = nullptr;
    if (CD3D12View::Resource)
    {
        NativeResource = CD3D12View::Resource->GetResource();
    }

    GetDevice()->CreateUnorderedAccessView(NativeResource, NativeCounterResource, &Desc, OfflineHandle);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseRenderTargetView

CD3D12RHIBaseRenderTargetView::CD3D12RHIBaseRenderTargetView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12RHIBaseRenderTargetView::CreateView(CD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    Assert(InResource != nullptr);
    Assert(OfflineHandle != 0);

    Desc = InDesc;

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);
    GetDevice()->CreateRenderTargetView(CD3D12View::Resource->GetResource(), &Desc, OfflineHandle);

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseDepthStencilView

CD3D12DepthStencilView::CD3D12DepthStencilView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12DepthStencilView::CreateView(CD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    Assert(InResource != nullptr);
    Assert(OfflineHandle != 0);

    Desc = InDesc;

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);
    GetDevice()->CreateDepthStencilView(CD3D12View::Resource->GetResource(), &Desc, OfflineHandle);

    return true;
}
