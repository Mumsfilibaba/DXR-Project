#include "D3D12Resource.h"
#include "D3D12Device.h"

D3D12Resource::D3D12Resource(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Resource(nullptr)
{
}

D3D12Resource::~D3D12Resource()
{
}

bool D3D12Resource::CreateResource(const D3D12_RESOURCE_DESC* Desc, D3D12_RESOURCE_STATES InitalState, EMemoryType MemoryType)
{
	// HeapProperties
	D3D12_HEAP_PROPERTIES HeapProperties = { };
	if (MemoryType == EMemoryType::MEMORY_TYPE_UPLOAD)
	{
		HeapProperties = 
		{
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0,
		};
	}
	else if (MemoryType == EMemoryType::MEMORY_TYPE_DEFAULT)
	{
		HeapProperties =
		{
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0,
		};
	}

	// Create
	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, Desc, InitalState, nullptr, IID_PPV_ARGS(&Resource));
	return SUCCEEDED(hResult);
}

void D3D12Resource::SetName(const std::string& Name)
{
	std::wstring WideName = ConvertToWide(Name);
	Resource->SetName(WideName.c_str());
}