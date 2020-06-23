#include "D3D12CommandQueue.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12Fence.h"

D3D12CommandQueue::D3D12CommandQueue(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Queue(nullptr)
{
}

D3D12CommandQueue::~D3D12CommandQueue()
{
}

bool D3D12CommandQueue::Initialize(D3D12_COMMAND_LIST_TYPE Type)
{
	// Create the command queue.
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.NodeMask	= 0;
	QueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	QueueDesc.Type		= Type;

	HRESULT hResult = Device->GetDevice()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12CommandQueue]: Created CommandQueue\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12CommandQueue]: Failed to create CommandQueue\n");
		return false;
	}
}

bool D3D12CommandQueue::SignalFence(D3D12Fence* InFence, Uint64 FenceValue)
{
	return SUCCEEDED(Queue->Signal(InFence->GetFence(), FenceValue));
}

void D3D12CommandQueue::ExecuteCommandList(D3D12CommandList* InCommandList)
{
	ID3D12CommandList* CommandLists[] = { InCommandList->GetCommandList() };
	Queue->ExecuteCommandLists(1, CommandLists);
}

void D3D12CommandQueue::SetName(const std::string& InName)
{
	Queue->SetName(ConvertToWide(InName).c_str());
}
