#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

D3D12CommandAllocator::D3D12CommandAllocator(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12CommandAllocator::~D3D12CommandAllocator()
{
}

bool D3D12CommandAllocator::Init(D3D12_COMMAND_LIST_TYPE Type)
{
	HRESULT hResult = Device->GetDevice()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12CommandAllocator]: Created CommandAllocator\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12CommandAllocator]: Failed to create CommandAllocator\n");
		return false;
	}
}

bool D3D12CommandAllocator::Reset()
{
	return Allocator->Reset();
}
