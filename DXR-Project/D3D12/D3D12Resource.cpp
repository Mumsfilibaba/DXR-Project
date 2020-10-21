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

VoidPtr D3D12Resource::Map(const Range& MappedRange)
{
	D3D12_RANGE MapRange = { MappedRange.Offset, MappedRange.Offset + MappedRange.Size };

	VoidPtr MappedData = nullptr;
	HRESULT HR = D3DResource->Map(0, &MapRange, &MappedData);
	if (FAILED(HR))
	{
		return nullptr;
	}

	return MappedData;
}

void D3D12Resource::Unmap(const Range& WrittenRange)
{
	D3D12_RANGE WriteRange = { WrittenRange.Offset, WrittenRange.Offset + WrittenRange.Size };
	D3DResource->Unmap(0, &WriteRange);
}