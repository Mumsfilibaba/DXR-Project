#include "D3D12Texture.h"
#include "D3D12Device.h"

D3D12Texture::D3D12Texture(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12Texture::~D3D12Texture()
{
}

bool D3D12Texture::Init(const TextureProperties& Properties)
{
	D3D12_RESOURCE_DESC Desc = {};
	Desc.DepthOrArraySize	= 1;
	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Format				= Properties.Format;
	Desc.Flags				= Properties.Flags;
	Desc.Width				= Properties.Width;
	Desc.Height				= Properties.Height;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	Desc.MipLevels			= 1;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality = 0;

	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&Properties.HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&Texture));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12Texture]: Created Texture\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12Texture]: FAILED to create Texture\n");
		return false;
	}
}
