#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

struct BufferProperties
{
	D3D12_RESOURCE_FLAGS	Flags;
	Uint64					SizeInBytes;
	D3D12_RESOURCE_STATES	InitalState;
	D3D12_HEAP_PROPERTIES	HeapProperties;
};

class D3D12Buffer : public D3D12DeviceChild
{
public:
	D3D12Buffer(D3D12Device* Device);
	~D3D12Buffer();

	bool Init(const BufferProperties& Properties);

	void* Map();
	void Unmap();

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress();

	ID3D12Resource1* GetResource() const
	{
		return Buffer.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource1> Buffer;
};