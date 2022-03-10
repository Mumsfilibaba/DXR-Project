#include "D3D12Device.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Views.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12View

CD3D12View::CD3D12View(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12DeviceObject(InDevice)
    , Resource(nullptr)
    , Heap(InHeap)
    , OfflineHandle({ 0 })
    , OnlineHandle({ 0 })
{
    D3D12_ERROR(Heap != nullptr, "Heap cannot be nullptr");
}

CD3D12View::~CD3D12View()
{
    InvalidateAndFreeHandle();
}

bool CD3D12View::AllocateHandle()
{
    D3D12_ERROR(Heap != nullptr, "Heap cannot be nullptr");

    OfflineHandle = Heap->Allocate(OfflineHeapIndex);
    if (OfflineHandle != 0)
    {
        const D3D12_DESCRIPTOR_HEAP_TYPE HeapType = Heap->GetType();
        if (HeapType != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            return true;
        }

        // For resource-handles we also retrieve the GPU-handle, this allow us to easier clear for example UAVs
        CD3D12DescriptorHeap* DescriptorHeap = Heap->GetDescriptorHeap(OfflineHeapIndex);
        if (DescriptorHeap)
        {
            OnlineHandle = DescriptorHeap->GetGpuDescriptorHandleFromCpuHandle(OfflineHandle);
            return true;
        }
    }
    
    return false;
}

void CD3D12View::InvalidateAndFreeHandle()
{
    D3D12_ERROR(Heap != nullptr, "Heap cannot be nullptr");

    Heap->Free(OfflineHandle, OfflineHeapIndex);

    OfflineHandle    = { 0 };
    OnlineHandle     = { 0 };
    OfflineHeapIndex = 0;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ConstantBufferView

CD3D12ConstantBufferView::CD3D12ConstantBufferView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12ConstantBufferView::CreateView(CD3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
    D3D12_ERROR(OfflineHandle != 0, "No descriptor handle has been allocated");

    Resource = MakeSharedRef<CD3D12Resource>(InResource);
    Desc     = InDesc;
    GetDevice()->CreateConstantBufferView(&Desc, OfflineHandle);

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseShaderResourceView

CD3D12BaseShaderResourceView::CD3D12BaseShaderResourceView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12BaseShaderResourceView::CreateView(CD3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    D3D12_ERROR(OfflineHandle != 0, "No descriptor handle has been allocated");

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);
    Desc = InDesc;

    ID3D12Resource* NativeResource = nullptr;
    if (CD3D12View::Resource)
    {
        NativeResource = CD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->CreateShaderResourceView(NativeResource, &Desc, OfflineHandle);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseUnorderedAccessView

CD3D12BaseUnorderedAccessView::CD3D12BaseUnorderedAccessView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
    , CounterResource(nullptr)
{
}

bool CD3D12BaseUnorderedAccessView::CreateView(CD3D12Resource* InCounterResource, CD3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    D3D12_ERROR(OfflineHandle != 0, "No descriptor handle has been allocated");

    CounterResource = InCounterResource;
    Desc            = InDesc;

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);

    ID3D12Resource* NativeCounterResource = nullptr;
    if (CounterResource)
    {
        NativeCounterResource = CounterResource->GetD3D12Resource();
    }

    ID3D12Resource* NativeResource = nullptr;
    if (CD3D12View::Resource)
    {
        NativeResource = CD3D12View::Resource->GetD3D12Resource();
    }

    GetDevice()->CreateUnorderedAccessView(NativeResource, NativeCounterResource, &Desc, OfflineHandle);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRenderTargetView

CD3D12BaseRenderTargetView::CD3D12BaseRenderTargetView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12BaseRenderTargetView::CreateView(CD3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
    D3D12_ERROR(InResource != nullptr, "Resource cannot be nullptr");
    D3D12_ERROR(OfflineHandle != 0, "No descriptor handle has been allocated");

    Desc = InDesc;

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);
    GetDevice()->CreateRenderTargetView(CD3D12View::Resource->GetD3D12Resource(), &Desc, OfflineHandle);

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseDepthStencilView

CD3D12BaseDepthStencilView::CD3D12BaseDepthStencilView(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap)
    : CD3D12View(InDevice, InHeap)
    , Desc()
{
}

bool CD3D12BaseDepthStencilView::CreateView(CD3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    D3D12_ERROR(InResource != nullptr, "Resource cannot be nullptr");
    D3D12_ERROR(OfflineHandle != 0   , "No descriptor handle has been allocated");

    Desc = InDesc;

    CD3D12View::Resource = MakeSharedRef<CD3D12Resource>(InResource);
    GetDevice()->CreateDepthStencilView(CD3D12View::Resource->GetD3D12Resource(), &Desc, OfflineHandle);

    return true;
}
