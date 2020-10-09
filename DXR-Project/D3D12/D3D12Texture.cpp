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

/*
* D3D12Texture1D
*/

D3D12Texture1D::D3D12Texture1D(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
{
}

D3D12Texture1D::~D3D12Texture1D()
{
}

bool D3D12Texture1D::Initialize(const Texture1DInitializer& InInitializer)
{
	D3D12_RESOURCE_DESC ResourceDesc = {};
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.DepthOrArraySize	= 1;
	ResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	ResourceDesc.Format				= ;
	ResourceDesc.Flags				= ;
	ResourceDesc.Width				= InInitializer.Width;
	ResourceDesc.Height				= 1;
	ResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels			= InInitializer.MipLevels;
	ResourceDesc.SampleDesc.Count	= 1;

	bool Result = D3D12Resource::Initialize(ResourceDesc, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, nullptr);
	if (Result)
	{
		Initializer = InInitializer;
		LOG_ERROR("[D3D12Texture1D]: Failed to create resource");
	}

	return Result;
}

/*
* D3D12Texture2D
*/

D3D12Texture2D::D3D12Texture2D(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
{
}

D3D12Texture2D::~D3D12Texture2D()
{
}

bool D3D12Texture2D::Initialize(const Texture2DInitializer& InInitializer)
{
	D3D12_RESOURCE_DESC ResourceDesc = {};
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.DepthOrArraySize	= 1;
	ResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Format				= ;
	ResourceDesc.Flags				= ;
	ResourceDesc.Width				= InInitializer.Width;
	ResourceDesc.Height				= InInitializer.Height;
	ResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels			= InInitializer.MipLevels;
	ResourceDesc.SampleDesc.Count	= InInitializer.SampleCount;

	if (ResourceDesc.SampleDesc.Count > 1)
	{
		ResourceDesc.SampleDesc.Quality = Device->GetMultisampleQuality(ResourceDesc.Format, ResourceDesc.SampleDesc.Count);
		if (ResourceDesc.SampleDesc.Quality == 0)
		{
			LOG_ERROR("[D3D12Texture2D]: Format does not support specified SampleCount=%u", ResourceDesc.SampleDesc.Count);
			return false;
		}
	}
	else
	{
		ResourceDesc.SampleDesc.Quality = 0;
	}

	bool Result = D3D12Resource::Initialize(ResourceDesc, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, nullptr);
	if (Result)
	{
		Initializer = InInitializer;
		LOG_ERROR("[D3D12Texture2D]: Failed to create resource");
	}

	return Result;
}

/*
* D3D12Texture2DArray
*/

D3D12Texture2DArray::D3D12Texture2DArray(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
{
}

D3D12Texture2DArray::~D3D12Texture2DArray()
{
}

bool D3D12Texture2DArray::Initialize(const Texture2DArrayInitializer& InInitializer)
{
	D3D12_RESOURCE_DESC ResourceDesc = {};
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.DepthOrArraySize	= 1;
	ResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Format				= ;
	ResourceDesc.Flags				= ;
	ResourceDesc.Width				= InInitializer.Width;
	ResourceDesc.Height				= InInitializer.Height;
	ResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels			= InInitializer.MipLevels;
	ResourceDesc.SampleDesc.Count	= InInitializer.SampleCount;

	if (ResourceDesc.SampleDesc.Count > 1)
	{
		ResourceDesc.SampleDesc.Quality = Device->GetMultisampleQuality(ResourceDesc.Format, ResourceDesc.SampleDesc.Count);
		if (ResourceDesc.SampleDesc.Quality == 0)
		{
			LOG_ERROR("[D3D12Texture2DArray]: Format does not support specified SampleCount=%u", ResourceDesc.SampleDesc.Count);
			return false;
		}
	}
	else
	{
		ResourceDesc.SampleDesc.Quality = 0;
	}

	bool Result = D3D12Resource::Initialize(ResourceDesc, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, nullptr);
	if (Result)
	{
		Initializer = InInitializer;
		LOG_ERROR("[D3D12Texture2DArray]: Failed to create resource");
	}

	return Result;
}
