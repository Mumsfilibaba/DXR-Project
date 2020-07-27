#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

D3D12CommandAllocator::D3D12CommandAllocator(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Allocator(nullptr)
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
		LOG_INFO("[D3D12CommandAllocator]: Created CommandAllocator");
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12CommandAllocator]: FAILED to create CommandAllocator");
		return false;
	}
}

bool D3D12CommandAllocator::Reset()
{
	return SUCCEEDED(Allocator->Reset());
}

void D3D12CommandAllocator::SetName(const std::string& Name)
{
	Allocator->SetName(ConvertToWide(Name).c_str());
}
