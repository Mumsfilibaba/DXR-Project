#include "D3D12Resource.h"
#include "D3D12Device.h"

/*
* D3D12Resource
*/

D3D12Resource::D3D12Resource(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, HeapType(D3D12_HEAP_TYPE_DEFAULT)
	, ResourceState(D3D12_RESOURCE_STATE_COMMON)
	, Desc()
	, Address(0)
{
}

D3D12Resource::~D3D12Resource()
{
}

Void* D3D12Resource::Map(const Range* MappedRange)
{
	Void* MappedData = nullptr;

	HRESULT hr = 0;
	if (MappedRange)
	{
		D3D12_RANGE MapRange = { MappedRange->Offset, MappedRange->Offset + MappedRange->Size };
		hr = D3DResource->Map(0, &MapRange, &MappedData);
	}
	else
	{
		hr = D3DResource->Map(0, nullptr, &MappedData);
	}

	if (FAILED(hr))
	{
		LOG_ERROR("[D3D12Resource::Map] Failed");
		return nullptr;
	}
	else
	{
		return MappedData;
	}
}

void D3D12Resource::Unmap(const Range* WrittenRange)
{
	if (WrittenRange)
	{
		D3D12_RANGE WriteRange = { WrittenRange->Offset, WrittenRange->Offset + WrittenRange->Size };
		D3DResource->Unmap(0, &WriteRange);
	}
	else
	{
		D3DResource->Unmap(0, nullptr);
	}
}