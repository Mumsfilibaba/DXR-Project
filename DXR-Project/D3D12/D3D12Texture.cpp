#include "D3D12Texture.h"
#include "D3D12Device.h"

D3D12Texture::D3D12Texture(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
	, RenderTargetView()
	, DepthStencilView()
{
}

D3D12Texture::~D3D12Texture()
{
}

bool D3D12Texture::Initialize(const TextureProperties& Properties)
{
	D3D12_RESOURCE_DESC Desc = {};
	Desc.DepthOrArraySize	= Properties.ArrayCount;
	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Format				= Properties.Format;
	Desc.Flags				= Properties.Flags;
	Desc.Width				= Properties.Width;
	Desc.Height				= Properties.Height;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	Desc.MipLevels			= Properties.MipLevels;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality = 0;

	if (CreateResource(&Desc, Properties.InitalState, Properties.MemoryType))
	{
		SetName(Properties.DebugName);

		::OutputDebugString("[D3D12Texture]: Created Texture\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12Texture]: FAILED to create Texture\n");
		return false;
	}
}
