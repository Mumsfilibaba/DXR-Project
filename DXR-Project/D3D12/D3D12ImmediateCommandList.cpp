#include "D3D12ImmediateCommandList.h"
#include "D3D12Device.h"

D3D12ImmediateCommandList::D3D12ImmediateCommandList(D3D12Device* InDevice)
	: D3D12CommandList(InDevice)
	, Queue(nullptr)
	, Fence(nullptr)
	, Allocators()
{
}

D3D12ImmediateCommandList::~D3D12ImmediateCommandList()
{
	WaitForCompletion();

	::CloseHandle(Event);
}

bool D3D12ImmediateCommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type)
{
	const Uint32 NumAllocators = 3;
	Allocators.Resize(NumAllocators);
	FenceValues.Resize(NumAllocators, 0);

	// Create allocators
	HRESULT hr = 0;
	for (Uint32 i = 0; i < NumAllocators; i++)
	{
		Device->GetDevice()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocators[i]));
		if (FAILED(hr))
		{
			LOG_ERROR("[D3D12ImmediateCommandList]: FAILED to create CommandAllocator");
			return false;
		}
	}

	LOG_INFO("[D3D12ImmediateCommandList]: Created CommandAllocators");

	// Create list
	hr = Device->GetDevice()->CreateCommandList(0, Type, Allocators[CurrentAllocatorIndex].Get(), nullptr, IID_PPV_ARGS(&CommandList));
	if (SUCCEEDED(hr))
	{
		//CommandList->Close();
		LOG_INFO("[D3D12ImmediateCommandList]: Created CommandList");

		if (FAILED(CommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
		{
			LOG_ERROR("[D3D12ImmediateCommandList]: FAILED to retrive DXR-CommandList");
			return false;
		}
	}
	else
	{
		LOG_ERROR("[D3D12ImmediateCommandList]: FAILED to create CommandList");
		return false;
	}

	// Create queue.
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.NodeMask	= 0;
	QueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	QueueDesc.Type		= Type;

	hr = Device->GetDevice()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D12ImmediateCommandList]: FAILED to create CommandQueue");
		return false;
	}
	else
	{
		LOG_INFO("[D3D12ImmediateCommandList]: Created CommandQueue");
	}

	// Create Fence and Event
	hr = Device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	if (SUCCEEDED(hr))
	{
		LOG_INFO("[D3D12ImmediateCommandList]: Created Fence");

		Event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (Event == NULL)
		{
			LOG_ERROR("[D3D12ImmediateCommandList]: FAILED to create Event");
			return false;
		}
		else
		{
			LOG_INFO("[D3D12ImmediateCommandList]: Created Event");
		}
	}
	else
	{
		LOG_INFO("[D3D12ImmediateCommandList]: FAILED to create Fence");
		return false;
	}

	return CreateUploadBuffer();
}

void D3D12ImmediateCommandList::Flush()
{
	// Execute Commandlist
	D3D12CommandList::Close();

	ID3D12CommandList* CommandLists[] = { CommandList.Get() };
	Queue->ExecuteCommandLists(1, CommandLists);

	CurrentFenceValue++;
	Queue->Signal(Fence.Get(), CurrentFenceValue);
	FenceValues[CurrentAllocatorIndex] = CurrentFenceValue;

	// Get next allocator
	CurrentAllocatorIndex++;
	if (CurrentAllocatorIndex >= Allocators.GetSize())
	{
		CurrentAllocatorIndex = 0;
	}

	// Make sure this allocator is not in use
	const Uint64 SyncValue = FenceValues[CurrentAllocatorIndex];
	WaitForValue(SyncValue);

	// Reset commandlist
	ID3D12CommandAllocator* Allocator = Allocators[CurrentAllocatorIndex].Get();
	Allocator->Reset();
	CommandList->Reset(Allocator, nullptr);

	// Reset NumDrawcalls
	NumDrawCalls = 0;
}

void D3D12ImmediateCommandList::WaitForCompletion()
{
	WaitForValue(CurrentFenceValue);

	ReleaseDeferredResources();
}

void D3D12ImmediateCommandList::WaitForValue(Uint64 FenceValue)
{
	if (FenceValue > 0)
	{
		HRESULT hr = Fence->SetEventOnCompletion(FenceValue, Event);
		if (SUCCEEDED(hr))
		{
			WaitForSingleObjectEx(Event, INFINITE, FALSE);
		}
		else
		{
			LOG_ERROR("[D3D12ImmediateCommandList] Failed to set WaitEvent");
		}
	}
}

void D3D12ImmediateCommandList::SetDebugName(const std::string& DebugName)
{
	using namespace Microsoft::WRL;

	D3D12CommandList::SetDebugName(DebugName);

	const std::wstring WideDebugName = ConvertToWide(DebugName);
	
	std::wstring QueueName = L"[Queue]" + WideDebugName;
	Queue->SetName(QueueName.c_str());

	std::wstring FenceName = L"[Fence]" + WideDebugName;
	Fence->SetName(FenceName.c_str());

	Uint32 Index = 0;
	std::wstring AllocatorName;
	for (ComPtr<ID3D12CommandAllocator>& Allocator : Allocators)
	{
		AllocatorName = L"[Allocator[" + std::to_wstring(Index) + L"]]" + WideDebugName;
		Allocator->SetName(AllocatorName.c_str());

		Index++;
	}
}
