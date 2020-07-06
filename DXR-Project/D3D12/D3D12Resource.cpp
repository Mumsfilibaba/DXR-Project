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

bool D3D12Resource::CreateResource(const D3D12_RESOURCE_DESC* InDesc, D3D12_RESOURCE_STATES InitalState, EMemoryType MemoryType)
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
	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, InDesc, InitalState, nullptr, IID_PPV_ARGS(&Resource));
	if (SUCCEEDED(hResult))
	{
		Desc = *InDesc;
		return true;
	}
	else
	{
		return false;
	}
}

bool D3D12Resource::Initialize(ID3D12Resource* InResource)
{
	VALIDATE(InResource != nullptr);

	Resource = InResource;
	Desc = Resource->GetDesc();

	return Resource != nullptr;
}

void D3D12Resource::SetShaderResourceView(std::shared_ptr<D3D12ShaderResourceView> InShaderResourceView, const Uint32 SubresourceIndex)
{
	if (SubresourceIndex >= ShaderResourceViews.size())
	{
		ShaderResourceViews.resize(SubresourceIndex + 1);
	}

	ShaderResourceViews[SubresourceIndex] = InShaderResourceView;
}

void D3D12Resource::SetUnorderedAccessView(std::shared_ptr<D3D12UnorderedAccessView> InUnorderedAccessView, const Uint32 SubresourceIndex)
{
	if (SubresourceIndex >= UnorderedAccessViews.size())
	{
		UnorderedAccessViews.resize(SubresourceIndex + 1);
	}

	UnorderedAccessViews[SubresourceIndex] = InUnorderedAccessView;
}

void D3D12Resource::SetName(const std::string& Name)
{
	std::wstring WideName = ConvertToWide(Name);
	Resource->SetName(WideName.c_str());
}