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
		::OutputDebugString("[D3D12CommandQueue]: Failed to create CommandQueue\n");
		return false;
	}
	else
	{
		::OutputDebugString("[D3D12CommandQueue]: Created CommandQueue\n");
	}

	QueueFence = std::make_unique<D3D12Fence>(Device);
	if (!QueueFence->Initialize(FenceValue))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool D3D12CommandQueue::SignalFence(D3D12Fence* Fence, Uint64 FenceValue)
{
	return SUCCEEDED(Queue->Signal(Fence->GetFence(), FenceValue));
}

bool D3D12CommandQueue::WaitForFence(D3D12Fence* Fence, Uint64 FenceValue)
{
	return SUCCEEDED(Queue->Wait(Fence->GetFence(), FenceValue));
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
	SignalFence(QueueFence.get(), FenceValue);
}

void D3D12CommandQueue::SetName(const std::string& Name)
{
	std::wstring WideName = ConvertToWide(Name);
	Queue->SetName(WideName.c_str());
}
