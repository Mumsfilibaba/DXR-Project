#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

D3D12CommandAllocator::D3D12CommandAllocator(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
{
}

D3D12CommandAllocator::~D3D12CommandAllocator()
{
}

bool D3D12CommandAllocator::Initialize(D3D12_COMMAND_LIST_TYPE Type)
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

void D3D12CommandAllocator::SetName(const std::string& InName)
{
	Allocator->SetName(ConvertToWide(InName).c_str());
}
