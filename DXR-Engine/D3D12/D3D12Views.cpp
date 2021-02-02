#include "D3D12Views.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

D3D12View::D3D12View(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
    : D3D12DeviceChild(InDevice)
    , DxResource(nullptr)
    , Heap(InHeap)
    , OfflineHandle({ 0 })
{
    VALIDATE(Heap != nullptr);
}

D3D12View::~D3D12View()
{
    // NOTE: Does not follow the rest of the engine's explicit alloc/dealloc, Ok for now
    Heap->Free(OfflineHandle, OfflineHeapIndex);
}

Bool D3D12View::Init()
{
    OfflineHandle = Heap->Allocate(OfflineHeapIndex);
    return OfflineHandle != 0;
}

D3D12ConstantBufferView::D3D12ConstantBufferView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
    : D3D12View(InDevice, InHeap)
    , Desc()
{
}

Bool D3D12ConstantBufferView::CreateView(const D3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    VALIDATE(InResource != nullptr);
    VALIDATE(OfflineHandle != 0);

    DxResource = InResource;
    Desc       = InDesc;
    Device->CreateConstantBufferView(&Desc, OfflineHandle);

    return true;
}

D3D12ShaderResourceView::D3D12ShaderResourceView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
    : D3D12View(InDevice, InHeap)
    , Desc()
{
}

Bool D3D12ShaderResourceView::CreateView(const D3D12Resource* InResource,  const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    VALIDATE(OfflineHandle != 0);

    DxResource = InResource;
    Desc       = InDesc;
    
    ID3D12Resource* NativeResource = nullptr;
    if (DxResource)
    {
        NativeResource = DxResource->GetResource();
    }

    Device->CreateShaderResourceView(NativeResource, &Desc, OfflineHandle);

    return true;
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
    : D3D12View(InDevice, InHeap)
    , Desc()
    , DxCounterResource(nullptr)
{
}

Bool D3D12UnorderedAccessView::CreateView(const D3D12Resource* InCounterResource, const D3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    VALIDATE(OfflineHandle != 0);

    Desc              = InDesc;
    DxCounterResource = InCounterResource;
    DxResource        = InResource;

    ID3D12Resource* NativeCounterResource = nullptr;
    if (DxCounterResource)
    {
        NativeCounterResource = DxCounterResource->GetResource();
    }

    ID3D12Resource* NativeResource = nullptr;
    if (DxResource)
    {
        NativeResource = DxResource->GetResource();
    }

    Device->CreateUnorderedAccessView(NativeResource, NativeCounterResource, &Desc, OfflineHandle);
    return true;
}

D3D12RenderTargetView::D3D12RenderTargetView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
    : D3D12View(InDevice, InHeap)
    , Desc()
{
}

Bool D3D12RenderTargetView::CreateView(
    const D3D12Resource* InResource, 
    const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    VALIDATE(InResource != nullptr);
    VALIDATE(OfflineHandle != 0);

    Desc       = InDesc;
    DxResource = InResource;
    Device->GetDevice()->CreateRenderTargetView(DxResource->GetResource(), &Desc, OfflineHandle);

    return true;
}

D3D12DepthStencilView::D3D12DepthStencilView(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap)
    : D3D12View(InDevice, InHeap)
    , Desc()
{
}

Bool D3D12DepthStencilView::CreateView(const D3D12Resource* InResource,  const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    VALIDATE(InResource != nullptr);
    VALIDATE(OfflineHandle != 0);

    Desc       = InDesc;
    DxResource = InResource;
    Device->GetDevice()->CreateDepthStencilView(DxResource->GetResource(), &Desc, OfflineHandle);

    return true;
}
