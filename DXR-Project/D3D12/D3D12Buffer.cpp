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

	D3D12_RESOURCE_DESC Desc	= {};
	Desc.DepthOrArraySize		= 1;
	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags					= Properties.Flags;
	Desc.Format					= DXGI_FORMAT_UNKNOWN;
	Desc.Height					= 1;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= 1;
	Desc.SampleDesc.Count		= 1;
	Desc.SampleDesc.Quality		= 0;
	Desc.Width					= SizeInBytes;

	if (CreateResource(&Desc, Properties.InitalState, Properties.MemoryType))
	{
		::OutputDebugString("[D3D12Buffer]: Created Buffer\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12Buffer]: FAILED to create Buffer\n");
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
