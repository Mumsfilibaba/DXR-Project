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
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.DepthOrArraySize	= Properties.ArrayCount;
	ResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Format				= Properties.Format;
	ResourceDesc.Flags				= Properties.Flags;
	ResourceDesc.Width				= Properties.Width;
	ResourceDesc.Height				= Properties.Height;
	ResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels			= Properties.MipLevels;
	ResourceDesc.SampleDesc.Count	= 1;
	ResourceDesc.SampleDesc.Quality = 0;

	if (CreateResource(&ResourceDesc, Properties.OptimizedClearValue, Properties.InitalState, Properties.MemoryType))
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
