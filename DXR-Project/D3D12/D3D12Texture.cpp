#include "D3D12Texture.h"
#include "D3D12Device.h"

D3D12Texture::D3D12Texture(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
	, RenderTargetViews()
	, DepthStencilViews()
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
	ResourceDesc.SampleDesc.Count	= Properties.SampleCount;

	if (ResourceDesc.SampleDesc.Count > 1)
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data = { };
		Data.Flags			= D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		Data.Format			= ResourceDesc.Format;
		Data.SampleCount	= ResourceDesc.SampleDesc.Count;
		HRESULT hr = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
		if (FAILED(hr))
		{
			return false;
		}

		ResourceDesc.SampleDesc.Quality = Data.NumQualityLevels - 1;
	}
	else
	{
		ResourceDesc.SampleDesc.Quality = 0;
	}

	if (CreateResource(&ResourceDesc, Properties.OptimizedClearValue, Properties.InitalState, Properties.MemoryType))
	{
		SetDebugName(Properties.DebugName);

		LOG_INFO("[D3D12Texture]: Created Texture");
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12Texture]: FAILED to create Texture");
		return false;
	}
}

void D3D12Texture::SetRenderTargetView(TSharedPtr<D3D12RenderTargetView> InRenderTargetView, Uint32 SubresourceIndex)
{
	if (SubresourceIndex >= RenderTargetViews.Size())
	{
		RenderTargetViews.Resize(SubresourceIndex + 1);
	}

	RenderTargetViews[SubresourceIndex] = InRenderTargetView;
}

void D3D12Texture::SetDepthStencilView(TSharedPtr<D3D12DepthStencilView> InDepthStencilView, Uint32 SubresourceIndex)
{
	if (SubresourceIndex >= DepthStencilViews.Size())
	{
		DepthStencilViews.Resize(SubresourceIndex + 1);
	}

	DepthStencilViews[SubresourceIndex] = InDepthStencilView;
}
