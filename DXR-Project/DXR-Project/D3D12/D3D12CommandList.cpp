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
		CommandList->Close();

		::OutputDebugString("[D3D12CommandList]: Created CommandList\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12CommandList]: Failed to create CommandList\n");
		return false;
	}
}

bool D3D12CommandList::Reset(D3D12CommandAllocator* Allocator)
{
	return SUCCEEDED(CommandList->Reset(Allocator->GetAllocator(), nullptr));
}

bool D3D12CommandList::Close()
{
	return SUCCEEDED(CommandList->Close());
}

void D3D12CommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE View, const Float32 ClearColor[4])
{
	CommandList->ClearRenderTargetView(View, ClearColor, 0, nullptr);
}

void D3D12CommandList::TransitionResourceState(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
	D3D12_RESOURCE_BARRIER Barrier = { };
	Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource	= Resource;
	Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Barrier.Transition.StateBefore	= BeforeState;
	Barrier.Transition.StateAfter	= AfterState;

	CommandList->ResourceBarrier(1, &Barrier);
}

