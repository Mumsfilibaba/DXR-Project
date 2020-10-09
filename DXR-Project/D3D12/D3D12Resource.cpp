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

bool D3D12Resource::Initialize(const D3D12_RESOURCE_DESC& InDesc, D3D12_RESOURCE_STATES InitalState, D3D12_HEAP_TYPE InHeapType, const D3D12_CLEAR_VALUE* OptimizedClearValue)
{
	D3D12_HEAP_PROPERTIES HeapProperties =
	{
		InHeapType,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	// Create
	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &InDesc, InitalState, OptimizedClearValue, IID_PPV_ARGS(&Resource));
	if (SUCCEEDED(hResult))
	{
		Desc			= InDesc;
		HeapType		= InHeapType;
		ResourceState	= InitalState;
		return true;
	}
	else
	{
		return false;
	}
}