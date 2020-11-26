#include "D3D12CommandQueue.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12Fence.h"

D3D12CommandQueue::D3D12CommandQueue(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Queue(nullptr)
	, QueueFence(nullptr)
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
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12CommandQueue]: FAILED to create CommandQueue");
		return false;
	}
	else
	{
		LOG_INFO("[D3D12CommandQueue]: Created CommandQueue");
	}

	QueueFence = MakeUnique<D3D12Fence>(Device);
	if (!QueueFence->Initialize(FenceValue))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool D3D12CommandQueue::SignalFence(D3D12Fence* Fence, uint64 InFenceValue)
{
	return SUCCEEDED(Queue->Signal(Fence->GetFence(), InFenceValue));
}

bool D3D12CommandQueue::WaitForFence(D3D12Fence* Fence, uint64 InFenceValue)
{
	return SUCCEEDED(Queue->Wait(Fence->GetFence(), InFenceValue));
}

void D3D12CommandQueue::WaitForCompletion()
{
	QueueFence->WaitForValue(FenceValue);
}

void D3D12CommandQueue::ExecuteCommandList(D3D12CommandList* CommandList)
{
	ID3D12CommandList* CommandLists[] = { CommandList->GetCommandList() };
	Queue->ExecuteCommandLists(1, CommandLists);

	FenceValue++;
	SignalFence(QueueFence.Get(), FenceValue);
}

void D3D12CommandQueue::SetDebugName(const std::string& DebugName)
{
	std::wstring WideDebugName = ConvertToWide(DebugName);
	Queue->SetName(WideDebugName.c_str());
}
