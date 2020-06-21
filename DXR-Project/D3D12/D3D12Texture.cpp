#include "D3D12Texture.h"
#include "D3D12Device.h"

D3D12Texture::D3D12Texture(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
{
}

D3D12Texture::~D3D12Texture()
{
}

bool D3D12Texture::Initialize(const TextureProperties& InProperties)
{
	D3D12_RESOURCE_DESC Desc = {};
	Desc.DepthOrArraySize	= 1;
	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Format				= InProperties.Format;
	Desc.Flags				= InProperties.Flags;
	Desc.Width				= InProperties.Width;
	Desc.Height				= InProperties.Height;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	Desc.MipLevels			= 1;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality = 0;

	HRESULT hResult = Device->GetDevice()->CreateCommittedResource(&InProperties.HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&Texture));
	if (SUCCEEDED(hResult))
	{
		SetName(InProperties.Name);

		::OutputDebugString("[D3D12Texture]: Created Texture\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12Texture]: FAILED to create Texture\n");
		return false;
	}
}

void D3D12Texture::SetName(const std::string& InName)
{
	Texture->SetName(ConvertToWide(InName).c_str());
}
