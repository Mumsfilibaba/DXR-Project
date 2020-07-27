#include "D3D12Buffer.h"
#include "D3D12Device.h"

#include <codecvt>
#include <locale>

D3D12Buffer::D3D12Buffer(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
{
}

D3D12Buffer::~D3D12Buffer()
{
}

bool D3D12Buffer::Initialize(const BufferProperties& Properties)
{
	SizeInBytes = Properties.SizeInBytes;

	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.DepthOrArraySize		= 1;
	ResourceDesc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags					= Properties.Flags;
	ResourceDesc.Format					= DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height					= 1;
	ResourceDesc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels				= 1;
	ResourceDesc.SampleDesc.Count		= 1;
	ResourceDesc.SampleDesc.Quality		= 0;
	ResourceDesc.Width					= SizeInBytes;

	if (CreateResource(&ResourceDesc, nullptr, Properties.InitalState, Properties.MemoryType))
	{
		LOG_INFO("[D3D12Buffer]: Created Buffer");
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12Buffer]: FAILED to create Buffer");
		return false;
	}
}

void* D3D12Buffer::Map()
{
	void* HostMemory = nullptr;
	if (SUCCEEDED(Resource->Map(0, nullptr, &HostMemory)))
	{
		return HostMemory;
	}
	else
	{
		return nullptr;
	}
}

void D3D12Buffer::Unmap()
{
	Resource->Unmap(0, nullptr);
}
