#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12CommandAllocator.h"

D3D12CommandList::D3D12CommandList(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12CommandList::~D3D12CommandList()
{
}

bool D3D12CommandList::Init(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline)
{
	HRESULT hResult = Device->GetDevice()->CreateCommandList(0, Type, Allocator->GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CommandList));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12CommandList]: Created CommandList\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12CommandList]: Failed to create CommandList\n");
		return false;
	}
}
