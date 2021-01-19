#pragma once
#include "D3D12RefCountedObject.h"

/*
* D3D12Fence
*/

class D3D12Fence : public D3D12RefCountedObject
{
public:
	inline D3D12Fence(D3D12Device* InDevice, ID3D12Fence* InFence, HANDLE InEvent)
		: D3D12RefCountedObject(InDevice)
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

	FORCEINLINE bool WaitForValue(UInt64 Value)
	{
		HRESULT hResult = Fence->SetEventOnCompletion(Value, Event);
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

	FORCEINLINE bool Signal(UInt64 Value)
	{
		HRESULT hResult = Fence->Signal(Value);
		return SUCCEEDED(hResult);
	}

	FORCEINLINE void SetName(const std::string& Name)
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