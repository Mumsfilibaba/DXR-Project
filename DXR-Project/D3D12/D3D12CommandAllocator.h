#pragma once
#include "D3D12DeviceChild.h"

class D3D12CommandAllocator : public D3D12DeviceChild
{
public:
	D3D12CommandAllocator(D3D12Device* InDevice);
	~D3D12CommandAllocator();

	bool Initialize(D3D12_COMMAND_LIST_TYPE InType);

	bool Reset();

	ID3D12CommandAllocator* GetAllocator() const
	{
		return Allocator.Get();
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
};