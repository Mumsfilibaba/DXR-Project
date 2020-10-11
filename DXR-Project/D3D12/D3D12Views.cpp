#include "D3D12Views.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

/*
* D3D12View
*/

D3D12View::D3D12View(D3D12Device* InDevice, ID3D12Resource* InResource)
	: D3D12DeviceChild(InDevice)
	, Resource(InResource)
	, OfflineHandle({ 0 })
{
}

D3D12View::~D3D12View()
{
	Heap->Free(OfflineHandle, OfflineHeapIndex);
}

void D3D12View::ResetResource()
{
	Resource.Reset();
}

/*
* D3D12ConstantBufferView
*/

D3D12ConstantBufferView::D3D12ConstantBufferView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
	: D3D12View(InDevice, InResource)
	, Desc()
{
	Heap = InDevice->GetGlobalResourceDescriptorHeap();

	OfflineHandle = Heap->Allocate(OfflineHeapIndex);
	CreateView(InResource, InDesc);
}

void D3D12ConstantBufferView::CreateView(ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc)
{
	Resource	= InResource;
	Desc		= InDesc;
	Device->CreateConstantBufferView(&Desc, OfflineHandle);
}

/*
* D3D12ShaderResourceView
*/

D3D12ShaderResourceView::D3D12ShaderResourceView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
	: D3D12View(InDevice, InResource)
	, Desc()
{
	Heap = InDevice->GetGlobalResourceDescriptorHeap();

	OfflineHandle = Heap->Allocate(OfflineHeapIndex);
	CreateView(InResource, InDesc);
}

void D3D12ShaderResourceView::CreateView(ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
	Resource	= InResource;
	Desc		= InDesc;

	Device->CreateShaderResourceView(InResource, &Desc, OfflineHandle);
}

/*
* D3D12UnorderedAccessView
*/

D3D12UnorderedAccessView::D3D12UnorderedAccessView(D3D12Device* InDevice, ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
	: D3D12View(InDevice, InResource)
	, Desc()
	, CounterResource()
{
	Heap = InDevice->GetGlobalResourceDescriptorHeap();

	OfflineHandle = Heap->Allocate(OfflineHeapIndex);
	CreateView(InCounterResource, InResource, InDesc);
}

void D3D12UnorderedAccessView::CreateView(
	ID3D12Resource* InCounterResource, 
	ID3D12Resource* InResource, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
	Desc			= InDesc;
	CounterResource	= InCounterResource;
	Resource		= InResource;

	Device->CreateUnorderedAccessView(InResource, InCounterResource, &Desc, OfflineHandle);
}


/*
* D3D12RenderTargetView
*/

D3D12RenderTargetView::D3D12RenderTargetView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
	: D3D12View(InDevice, InResource)
	, Desc()
{
	Heap = InDevice->GetGlobalRenderTargetDescriptorHeap();

	OfflineHandle = Heap->Allocate(OfflineHeapIndex);
	CreateView(InResource, InDesc);
}

void D3D12RenderTargetView::CreateView(ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc)
{
	Desc		= InDesc;
	Resource	= InResource;
	Device->GetDevice()->CreateRenderTargetView(InResource, &Desc, OfflineHandle);
}

/*
* D3D12DepthStencilView
*/

D3D12DepthStencilView::D3D12DepthStencilView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
	: D3D12View(InDevice, InResource)
	, Desc()
{
	Heap = InDevice->GetGlobalDepthStencilDescriptorHeap();

	OfflineHandle = Heap->Allocate(OfflineHeapIndex);
	CreateView(InResource, InDesc);
}

void D3D12DepthStencilView::CreateView(ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
	Desc		= InDesc;
	Resource	= InResource;
	Device->GetDevice()->CreateDepthStencilView(InResource, &Desc, OfflineHandle);
}
