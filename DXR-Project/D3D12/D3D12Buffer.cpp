#include "D3D12Buffer.h"
#include "D3D12Device.h"

D3D12Buffer::D3D12Buffer(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12Buffer::~D3D12Buffer()
{
}

bool D3D12Buffer::Init(D3D12_RESOURCE_FLAGS Flags, Uint64 SizeInBytes, D3D12_RESOURCE_STATES InitalState, D3D12_HEAP_PROPERTIES HeapProperties)
{
	D3D12_RESOURCE_DESC Desc = {};
	Desc.DepthOrArraySize	= 1;
	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags				= Flags;
	Desc.Format				= DXGI_FORMAT_UNKNOWN;
	Desc.Height				= 1;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels			= 1;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Width				= SizeInBytes;

	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InitalState, nullptr, IID_PPV_ARGS(&Buffer));
	if (SUCCEEDED(hResult))
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
	if (SUCCEEDED(Buffer->Map(0, nullptr, &HostMemory)))
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
	Buffer->Unmap(0, nullptr);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetVirtualAddress()
{
	return Buffer->GetGPUVirtualAddress();
}
