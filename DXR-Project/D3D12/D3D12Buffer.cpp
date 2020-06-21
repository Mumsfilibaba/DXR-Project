#include "D3D12Buffer.h"
#include "D3D12Device.h"

#include <codecvt>
#include <locale>

D3D12Buffer::D3D12Buffer(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
{
}

D3D12Buffer::~D3D12Buffer()
{
}

bool D3D12Buffer::Initialize(const BufferProperties& InProperties)
{
	D3D12_RESOURCE_DESC Desc = {};
	Desc.DepthOrArraySize	= 1;
	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags				= InProperties.Flags;
	Desc.Format				= DXGI_FORMAT_UNKNOWN;
	Desc.Height				= 1;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels			= 1;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Width				= InProperties.SizeInBytes;

	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&InProperties.HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InProperties.InitalState, nullptr, IID_PPV_ARGS(&Buffer));
	if (SUCCEEDED(hResult))
	{
		SetName(InProperties.Name);

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

void D3D12Buffer::SetName(const std::string& InName)
{
	Buffer->SetName(ConvertToWide(InName).c_str());
}
