#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

/*
* D3D12Fence
*/

class D3D12Fence : public D3D12DeviceChild
{
public:
	inline D3D12Fence(D3D12Device* InDevice, ID3D12Fence* InFence, HANDLE InEvent)
		: D3D12DeviceChild(InDevice)
		, Fence(InFence)
		, Event(InEvent)
	{
		VALIDATE(Fence != nullptr);
		VALIDATE(Event != 0);
	}

	inline ~D3D12Fence()
	{
		::CloseHandle(Event);
	}

	inline bool D3D12Fence::WaitForValue(Uint64 FenceValue)
	{
		HRESULT hResult = Fence->SetEventOnCompletion(FenceValue, Event);
		if (SUCCEEDED(hResult))
		{
			::WaitForSingleObjectEx(Event, INFINITE, FALSE);
			return true;
		}
		else
		{
			return false;
		}
	}

	FORCEINLINE void SetDebugName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		Fence->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12Fence* GetFence() const
	{
		return Fence.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
	HANDLE Event = 0;
};