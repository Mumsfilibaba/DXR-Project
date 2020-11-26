#include "D3D12Fence.h"
#include "D3D12Device.h"

D3D12Fence::D3D12Fence(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Fence(nullptr)
{
}

D3D12Fence::~D3D12Fence()
{
	::CloseHandle(Event);
}

bool D3D12Fence::Initialize(uint64 InitalValue)
{
	HRESULT hResult = Device->GetDevice()->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	if (SUCCEEDED(hResult))
	{
		LOG_INFO("[D3D12Fence]: Created Fence");

		Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (Event == NULL)
		{
			LOG_ERROR("[D3D12Fence]: FAILED to create Event");
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		LOG_INFO("[D3D12Fence]: FAILED to create Fence");
		return false;
	}
}

bool D3D12Fence::WaitForValue(uint64 FenceValue)
{
	HRESULT hResult = Fence->SetEventOnCompletion(FenceValue, Event);
	if (SUCCEEDED(hResult))
	{
		WaitForSingleObjectEx(Event, INFINITE, FALSE);
		return true;
	}
	else
	{
		return false;
	}
}

void D3D12Fence::SetDebugName(const std::string& DebugName)
{
	std::wstring WideDebugName = ConvertToWide(DebugName);
	Fence->SetName(WideDebugName.c_str());
}
