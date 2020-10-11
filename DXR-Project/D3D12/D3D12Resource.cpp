#include "D3D12Resource.h"
#include "D3D12Device.h"

D3D12Resource::D3D12Resource(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Resource(nullptr)
	, HeapType(D3D12_HEAP_TYPE_DEFAULT)
	, ResourceState(D3D12_RESOURCE_STATE_COMMON)
	, Desc()
	, Address(0)
{
}

D3D12Resource::~D3D12Resource()
{
}