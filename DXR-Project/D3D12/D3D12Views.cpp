#include "D3D12Views.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

/*
* D3D12View
*/

D3D12View::D3D12View(D3D12Device* InDevice, ID3D12Resource* InResource)
	: D3D12DeviceChild(InDevice)
	, Resource(InResource)
	, Descriptor()
{
}

void D3D12View::SetName(const std::string& Name)
{
	// Empty for now
}

/*
* D3D12ShaderResourceView
*/

D3D12ShaderResourceView::D3D12ShaderResourceView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* InDesc)
	: D3D12View(InDevice, InResource)
{
	Desc		= *InDesc;
	Descriptor	= InDevice->GetGlobalResourceDescriptorHeap()->Allocate();

	InDevice->GetDevice()->CreateShaderResourceView(InResource, InDesc, Descriptor.GetCPUHandle());
}

/*
* D3D12UnorderedAccessView
*/

D3D12UnorderedAccessView::D3D12UnorderedAccessView(D3D12Device* InDevice, ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* InDesc)
	: D3D12View(InDevice, InResource)
	, CounterResource(InCounterResource)
{
	Desc		= *InDesc;
	Descriptor	= InDevice->GetGlobalResourceDescriptorHeap()->Allocate();

	InDevice->GetDevice()->CreateUnorderedAccessView(InResource, InCounterResource, InDesc, Descriptor.GetCPUHandle());
}

/*
* D3D12RenderTargetView
*/

D3D12RenderTargetView::D3D12RenderTargetView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC* InDesc)
	: D3D12View(InDevice, InResource)
{
	Desc		= *InDesc;
	Descriptor	= InDevice->GetGlobalResourceDescriptorHeap()->Allocate();

	InDevice->GetDevice()->CreateRenderTargetView(InResource, InDesc, Descriptor.GetCPUHandle());
}

/*
* D3D12DepthStencilView
*/

D3D12DepthStencilView::D3D12DepthStencilView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* InDesc)
	: D3D12View(InDevice, InResource)
{
	Desc = *InDesc;
	Descriptor = InDevice->GetGlobalResourceDescriptorHeap()->Allocate();

	InDevice->GetDevice()->CreateDepthStencilView(InResource, InDesc, Descriptor.GetCPUHandle());
}
