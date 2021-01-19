#pragma once
#include "D3D12RefCountedObject.h"

/*
* D3D12CommandAllocator
*/

class D3D12CommandAllocator : public D3D12RefCountedObject
{
public:
	inline D3D12CommandAllocator(D3D12Device* InDevice, ID3D12CommandAllocator* InAllocator)
		: D3D12RefCountedObject(InDevice)
		, Allocator(InAllocator)
	{
	}

	FORCEINLINE bool Reset()
	{
		return SUCCEEDED(Allocator->Reset());
	}

	FORCEINLINE void SetName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		Allocator->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
	{
		return Allocator.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
};