#include "D3D12Resource.h"
#include "D3D12Device.h"

D3D12Resource::D3D12Resource(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, NativeResource(nullptr)
{
}

D3D12Resource::~D3D12Resource()
{
}

bool D3D12Resource::CreateResource(const D3D12_RESOURCE_DESC* InDesc, const D3D12_CLEAR_VALUE* OptimizedClearValue, D3D12_RESOURCE_STATES InitalState, EMemoryType InMemoryType)
{
	// HeapProperties
	D3D12_HEAP_PROPERTIES HeapProperties = { };
	if (InMemoryType == EMemoryType::MEMORY_TYPE_CPU_VISIBLE)
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
	else if (InMemoryType == EMemoryType::MEMORY_TYPE_GPU)
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
	else
	{
		Debug::DebugBreak();
	}

	// Create
	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, InDesc, InitalState, OptimizedClearValue, IID_PPV_ARGS(&NativeResource));
	if (SUCCEEDED(hResult))
	{
		Desc		= *InDesc;
		MemoryType	= InMemoryType;

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

	NativeResource = InResource;
	Desc = NativeResource->GetDesc();

	return NativeResource != nullptr;
}

void D3D12Resource::SetShaderResourceView(TSharedPtr<D3D12ShaderResourceView> InShaderResourceView, const Uint32 SubresourceIndex)
{
	if (SubresourceIndex >= ShaderResourceViews.Size())
	{
		ShaderResourceViews.Resize(SubresourceIndex + 1);
	}

	ShaderResourceViews[SubresourceIndex] = InShaderResourceView;
}

void D3D12Resource::SetUnorderedAccessView(TSharedPtr<D3D12UnorderedAccessView> InUnorderedAccessView, const Uint32 SubresourceIndex)
{
	if (SubresourceIndex >= UnorderedAccessViews.Size())
	{
		UnorderedAccessViews.Resize(SubresourceIndex + 1);
	}

	UnorderedAccessViews[SubresourceIndex] = InUnorderedAccessView;
}

void D3D12Resource::SetDebugName(const std::string& DebugName)
{
	std::wstring WideDebugName = ConvertToWide(DebugName);
	NativeResource->SetName(WideDebugName.c_str());
}