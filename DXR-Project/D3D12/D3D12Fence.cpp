#include "D3D12Fence.h"
#include "D3D12Device.h"

D3D12Fence::D3D12Fence(D3D12Device* Device)
	: D3D12DeviceChild(Device)
	, Fence(nullptr)
{
}

D3D12Fence::~D3D12Fence()
{
	::CloseHandle(Event);
}

bool D3D12Fence::Initialize(Uint64 InitalValue)
{
	HRESULT hResult = Device->GetDevice()->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12Fence]: Created Fence\n");

		Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (Event == NULL)
		{
			::OutputDebugString("[D3D12Fence]: Failed to create Event\n");
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		::OutputDebugString("[D3D12Fence]: Failed to create Fence\n");
		return false;
	}
}

bool D3D12Fence::WaitForValue(Uint64 FenceValue)
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

void D3D12Fence::SetName(const std::string& InName)
{
	Fence->SetName(ConvertToWide(InName).c_str());
}
