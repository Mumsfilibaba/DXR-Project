#pragma once
#include "D3D12DeviceChild.h"

enum class EMemoryType : Uint32
{
	MEMORY_TYPE_UNKNOWN	= 0,
	MEMORY_TYPE_UPLOAD	= 1,
	MEMORY_TYPE_DEFAULT	= 2,
};

class D3D12Resource : public D3D12DeviceChild
{
public:
	D3D12Resource(D3D12Device* InDevice);
	~D3D12Resource();

	FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
	{
		return Resource->GetGPUVirtualAddress();
	}

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;
	
protected:
	bool CreateResource(const D3D12_RESOURCE_DESC* Desc, D3D12_RESOURCE_STATES InitalState, EMemoryType MemoryType);

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
};