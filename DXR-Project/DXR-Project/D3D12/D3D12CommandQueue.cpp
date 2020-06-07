#include "D3D12CommandQueue.h"
#include "D3D12GraphicsDevice.h"

D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* Device)
	: Device(Device)
{
}

D3D12CommandQueue::~D3D12CommandQueue()
{
}

bool D3D12CommandQueue::Init()
{
	// Create the command queue.
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.NodeMask	= 0;
	QueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	QueueDesc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;

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
