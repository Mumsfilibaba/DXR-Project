#include "Containers/TUniquePtr.h"

#include "D3D12RenderingAPI.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12ComputePipelineState.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12GraphicsPipelineState.h"
#include "D3D12RayTracingPipelineState.h"
#include "D3D12RayTracingScene.h"
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12SwapChain.h"
#include "D3D12Helpers.h"

#include <algorithm>

/*
* D3D12RenderingAPI
*/

D3D12RenderingAPI::D3D12RenderingAPI()
	: RenderingAPI(ERenderingAPI::RenderingAPI_D3D12)
	, SwapChain(nullptr)
	, Device(nullptr)
	, Queue(nullptr)
	, ComputeQueue(nullptr)
{
}

D3D12RenderingAPI::~D3D12RenderingAPI()
{
	SwapChain.Reset();
}

bool D3D12RenderingAPI::Init(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug)
{
	// Create device
	Device = MakeShared<D3D12Device>();
	if (!Device->CreateDevice(EnableDebug, true))
	{
		return false;
	}

	if (Device->IsRayTracingSupported())
	{
		Device->InitRayTracing();
	}

	// TODO: Create CommandContext

	// Create swapchain
	SwapChain = Device->CreateSwapChain(StaticCast<WindowsWindow>(RenderWindow).Get(), Queue.Get());
	if (!SwapChain)
	{
		return false;
	}

	return true;
}

/*
* D3D12RenderingAPI - Resources
*/

Texture1D* D3D12RenderingAPI::CreateTexture1D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 MipLevels, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags	= ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType	= D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	Desc.Flags				= Flags;
	Desc.Format				= ConvertFormat(Format);
	Desc.Width				= Width;
	Desc.Height				= 1;
	Desc.DepthOrArraySize	= 1;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels			= MipLevels;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality	= 0;

	D3D12Texture1D* NewTexture = new D3D12Texture1D(Device.Get(), Format, Usage, Width, MipLevels, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

Texture1DArray* D3D12RenderingAPI::CreateTexture1DArray(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 MipLevels, 
	Uint32 ArrayCount, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags = ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	Desc.Flags				= Flags;
	Desc.Format				= ConvertFormat(Format);
	Desc.Width				= Width;
	Desc.Height				= 1;
	Desc.DepthOrArraySize	= ArrayCount;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels			= MipLevels;
	Desc.SampleDesc.Count	= 1;
	Desc.SampleDesc.Quality	= 0;

	D3D12Texture1DArray* NewTexture = new D3D12Texture1DArray(Device.Get(), Format, Usage, Width, MipLevels, ArrayCount, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

Texture2D* D3D12RenderingAPI::CreateTexture2D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 MipLevels, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags	= ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType	= D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Flags					= Flags;
	Desc.Format					= ConvertFormat(Format);
	Desc.Width					= Width;
	Desc.Height					= Height;
	Desc.DepthOrArraySize		= 1;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= MipLevels;
	Desc.SampleDesc.Count		= SampleCount;

	if (SampleCount > 1)
	{
		const Int32 Quality = Device->GetMultisampleQuality(Desc.Format, SampleCount);
		Desc.SampleDesc.Quality	= std::max<Int32>(Quality - 1, 0);
	}
	else
	{
		Desc.SampleDesc.Quality	= 0;
	}
	
	D3D12Texture2D* NewTexture = new D3D12Texture2D(Device.Get(), Format, Usage, Width, Height, MipLevels, SampleCount, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

Texture2DArray* D3D12RenderingAPI::CreateTexture2DArray(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 MipLevels, 
	Uint32 ArrayCount, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags	= ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType	= D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Flags					= Flags;
	Desc.Format					= ConvertFormat(Format);
	Desc.Width					= Width;
	Desc.Height					= Height;
	Desc.DepthOrArraySize		= ArrayCount;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= MipLevels;
	Desc.SampleDesc.Count		= SampleCount;

	if (SampleCount > 1)
	{
		const Int32 Quality = Device->GetMultisampleQuality(Desc.Format, SampleCount);
		Desc.SampleDesc.Quality	= std::max<Int32>(Quality - 1, 0);
	}
	else
	{
		Desc.SampleDesc.Quality	= 0;
	}

	D3D12Texture2DArray* NewTexture = new D3D12Texture2DArray(Device.Get(), Format, Usage, Width, Height, MipLevels, ArrayCount, SampleCount, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

TextureCube* D3D12RenderingAPI::CreateTextureCube(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Size, 
	Uint32 MipLevels, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags	= ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType	= D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;
	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Flags					= Flags;
	Desc.Format					= ConvertFormat(Format);
	Desc.Width					= Size;
	Desc.Height					= Size;
	Desc.DepthOrArraySize		= TEXTURE_CUBE_FACE_COUNT;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= MipLevels;
	Desc.SampleDesc.Count		= SampleCount;

	if (SampleCount > 1)
	{
		const Int32 Quality = Device->GetMultisampleQuality(Desc.Format, SampleCount);
		Desc.SampleDesc.Quality	= std::max<Int32>(Quality - 1, 0);
	}
	else
	{
		Desc.SampleDesc.Quality	= 0;
	}

	D3D12TextureCube* NewTexture = new D3D12TextureCube(Device.Get(), Format, Usage, Size, MipLevels, SampleCount, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

TextureCubeArray* D3D12RenderingAPI::CreateTextureCubeArray(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Size, 
	Uint32 MipLevels, 
	Uint32 ArrayCount, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags = ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;
	Desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Flags				= Flags;
	Desc.Format				= ConvertFormat(Format);
	Desc.Width				= Size;
	Desc.Height				= Size;
	Desc.DepthOrArraySize	= TEXTURE_CUBE_FACE_COUNT * ArrayCount;
	Desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels			= MipLevels;
	Desc.SampleDesc.Count	= SampleCount;

	if (SampleCount > 1)
	{
		const Int32 Quality = Device->GetMultisampleQuality(Desc.Format, SampleCount);
		Desc.SampleDesc.Quality = std::max<Int32>(Quality - 1, 0);
	}
	else
	{
		Desc.SampleDesc.Quality = 0;
	}

	D3D12TextureCubeArray* NewTexture = new D3D12TextureCubeArray(Device.Get(), Format, Usage, Size, MipLevels, ArrayCount, SampleCount, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

Texture3D* D3D12RenderingAPI::CreateTexture3D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 Depth, 
	Uint32 MipLevels, 
	const ClearValue& OptimizedClearValue) const
{
	D3D12_RESOURCE_FLAGS Flags	= ConvertTextureUsage(Usage);
	D3D12_HEAP_TYPE HeapType	= D3D12_HEAP_TYPE_DEFAULT;
	if (Usage & TextureUsage_Dynamic)
	{
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	Desc.Flags					= Flags;
	Desc.Format					= ConvertFormat(Format);
	Desc.Width					= Width;
	Desc.Height					= Height;
	Desc.DepthOrArraySize		= Depth;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= MipLevels;
	Desc.SampleDesc.Count		= 1;

	D3D12Texture3D* NewTexture = new D3D12Texture3D(Device.Get(), Format, Usage, Width, Height, Depth, MipLevels, OptimizedClearValue);
	if (!AllocateTexture(*NewTexture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewTexture, InitalData);
	}

	return NewTexture;
}

VertexBuffer* D3D12RenderingAPI::CreateVertexBuffer(
	const ResourceData* InitalData, 
	Uint32 SizeInBytes, 
	Uint32 VertexStride, 
	Uint32 Usage) const
{
	D3D12_RESOURCE_FLAGS Flags			= ConvertBufferUsage(Usage);
	D3D12_HEAP_TYPE HeapType			= D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES InitalState	= D3D12_RESOURCE_STATE_COMMON;
	if (Usage & BufferUsage_Dynamic)
	{
		InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
		HeapType	= D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12VertexBuffer* NewBuffer = new D3D12VertexBuffer(Device.Get(), SizeInBytes, VertexStride, Usage);
	if (!AllocateBuffer(*NewBuffer, HeapType, InitalState, Flags, SizeInBytes))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate buffer");
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewBuffer, InitalData);
	}

	D3D12_VERTEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	View.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	View.SizeInBytes	= SizeInBytes;
	View.StrideInBytes	= VertexStride;
	NewBuffer->VertexBufferView = View;

	return NewBuffer;
}

IndexBuffer* D3D12RenderingAPI::CreateIndexBuffer(
	const ResourceData* InitalData, 
	Uint32 SizeInBytes, 
	EIndexFormat IndexFormat, 
	Uint32 Usage) const
{
	D3D12_RESOURCE_FLAGS Flags			= ConvertBufferUsage(Usage);
	D3D12_HEAP_TYPE HeapType			= D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES InitalState	= D3D12_RESOURCE_STATE_COMMON;
	if (Usage & BufferUsage_Dynamic)
	{
		InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12IndexBuffer* NewBuffer = new D3D12IndexBuffer(Device.Get(), SizeInBytes, IndexFormat, Usage);
	if (!AllocateBuffer(*NewBuffer, HeapType, InitalState, Flags, SizeInBytes))
	{
		return nullptr;
	}
	
	if (InitalData)
	{
		UploadResource(*NewBuffer, InitalData);
	}

	D3D12_INDEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_INDEX_BUFFER_VIEW));

	View.BufferLocation = NewBuffer->GetGPUVirtualAddress();
	View.SizeInBytes	= SizeInBytes;
	if (IndexFormat == EIndexFormat::IndexFormat_Uint16)
	{
		View.Format	= DXGI_FORMAT_R16_UINT;
	}
	else if (IndexFormat == EIndexFormat::IndexFormat_Uint32)
	{
		View.Format = DXGI_FORMAT_R32_UINT;
	}

	NewBuffer->IndexBufferView = View;

	return NewBuffer;
}

ConstantBuffer* D3D12RenderingAPI::CreateConstantBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Usage) const
{
	const Uint32 AlignedSize = AlignUp<Uint32>(SizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	D3D12_RESOURCE_FLAGS Flags			= ConvertBufferUsage(Usage);
	D3D12_HEAP_TYPE HeapType			= D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES InitalState	= D3D12_RESOURCE_STATE_COMMON;
	if (Usage & BufferUsage_Dynamic)
	{
		InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12ConstantBuffer* NewBuffer = new D3D12ConstantBuffer(Device.Get(), SizeInBytes, Usage);
	if (!AllocateBuffer(*NewBuffer, HeapType, InitalState, Flags, AlignedSize))
	{
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewBuffer, InitalData);
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
	Memory::Memzero(&ViewDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));

	ViewDesc.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes	= SizeInBytes;

	D3D12ConstantBufferView* View = new D3D12ConstantBufferView(Device.Get(), NewBuffer, ViewDesc);
	NewBuffer->View = View;

	return NewBuffer;
}

StructuredBuffer* D3D12RenderingAPI::CreateStructuredBuffer(
	const ResourceData* InitalData, 
	Uint32 SizeInBytes, 
	Uint32 Stride, 
	Uint32 Usage) const
{

	D3D12_RESOURCE_FLAGS Flags			= ConvertBufferUsage(Usage);
	D3D12_HEAP_TYPE HeapType			= D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES InitalState	= D3D12_RESOURCE_STATE_COMMON;
	if (Usage & BufferUsage_Dynamic)
	{
		InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
		HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}

	D3D12StructuredBuffer* NewBuffer = new D3D12StructuredBuffer(Device.Get(), SizeInBytes, Stride, Usage);
	if (!AllocateBuffer(*NewBuffer, HeapType, InitalState, Flags, SizeInBytes))
	{
		return nullptr;
	}

	if (InitalData)
	{
		UploadResource(*NewBuffer, InitalData);
	}

	return NewBuffer;
}

RayTracingGeometry* D3D12RenderingAPI::CreateRayTracingGeometry() const
{
	return nullptr;
}

RayTracingScene* D3D12RenderingAPI::CreateRayTracingScene() const
{
	return nullptr;
}

/*
* D3D12RenderingAPI - Resource Views
*/

// ShaderResourceView
ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Buffer* Buffer, 
	Uint32 FirstElement, 
	Uint32 ElementCount,
	EFormat Format) const
{
	VALIDATE(Buffer != nullptr);
	VALIDATE(Buffer->HasShaderResourceUsage());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	Desc.Buffer.FirstElement		= FirstElement;
	Desc.Buffer.NumElements			= ElementCount;
	Desc.Format						= ConvertFormat(Format);
	Desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	Desc.Buffer.StructureByteStride	= 0;

	const D3D12Resource* Resource = D3D12BufferCast(Buffer);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified Buffer was not of buffer-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Buffer* Buffer,
	Uint32 FirstElement,
	Uint32 ElementCount,
	Uint32 Stride) const
{
	VALIDATE(Buffer != nullptr);
	VALIDATE(Buffer->HasShaderResourceUsage());
	VALIDATE(Stride != 0);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	Desc.Buffer.FirstElement		= FirstElement;
	Desc.Buffer.NumElements			= ElementCount;
	Desc.Format						= DXGI_FORMAT_UNKNOWN;
	Desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
	Desc.Buffer.StructureByteStride = 0;

	const D3D12Resource* Resource = D3D12BufferCast(Buffer);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified Buffer was not of buffer-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture1D* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE1D;
	Desc.Format							= ConvertFormat(Format);
	Desc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.Texture1D.MipLevels			= MipLevels;
	Desc.Texture1D.MostDetailedMip		= MostDetailedMip;
	Desc.Texture1D.ResourceMinLODClamp	= 0.0f;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture1DArray* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
	Desc.Format								= ConvertFormat(Format);
	Desc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.Texture1DArray.MipLevels			= MipLevels;
	Desc.Texture1DArray.MostDetailedMip		= MostDetailedMip;
	Desc.Texture1DArray.ArraySize			= ArraySize;
	Desc.Texture1DArray.FirstArraySlice		= FirstArraySlice;
	Desc.Texture1DArray.ResourceMinLODClamp	= 0.0f;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture2D* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.Format						= ConvertFormat(Format);
	Desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;	
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		Desc.Texture2D.MipLevels			= MipLevels;
		Desc.Texture2D.MostDetailedMip		= MostDetailedMip;
		Desc.Texture2D.PlaneSlice			= 0;
		Desc.Texture2D.ResourceMinLODClamp	= 0.0f;
	}
	else
	{
		Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture2DArray* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.Format						= ConvertFormat(Format);
	Desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.ArraySize			= ArraySize;
		Desc.Texture2DArray.MostDetailedMip		= MostDetailedMip;
		Desc.Texture2DArray.FirstArraySlice		= FirstArraySlice;
		Desc.Texture2DArray.MipLevels			= MipLevels;
		Desc.Texture2DArray.PlaneSlice			= 0;
		Desc.Texture2DArray.ResourceMinLODClamp	= 0.0f;
	}
	else
	{
		Desc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.ArraySize			= ArraySize;
		Desc.Texture2DMSArray.FirstArraySlice	= FirstArraySlice;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);
	VALIDATE(Texture->IsMultiSampled() == false);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.Format								= ConvertFormat(Format);
	Desc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURECUBE;
	Desc.TextureCube.MostDetailedMip		= MostDetailedMip;
	Desc.TextureCube.MipLevels				= MipLevels;
	Desc.TextureCube.ResourceMinLODClamp	= 0.0f;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const TextureCubeArray* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;

	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < (Texture->GetArrayCount() / TEXTURE_CUBE_FACE_COUNT));
	VALIDATE(Format != EFormat::Format_Unknown);
	VALIDATE(Texture->IsMultiSampled() == false);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.Format									= ConvertFormat(Format);
	Desc.Shader4ComponentMapping				= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.ViewDimension							= D3D12_SRV_DIMENSION_TEXTURECUBE;
	Desc.TextureCubeArray.MostDetailedMip		= MostDetailedMip;
	Desc.TextureCubeArray.MipLevels				= MipLevels;
	Desc.TextureCubeArray.First2DArrayFace		= FirstArraySlice * TEXTURE_CUBE_FACE_COUNT;
	Desc.TextureCubeArray.NumCubes				= ArraySize;
	Desc.TextureCubeArray.ResourceMinLODClamp	= 0.0f;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture3D* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.Format							= ConvertFormat(Format);
	Desc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE3D;
	Desc.Texture3D.MostDetailedMip		= MostDetailedMip;
	Desc.Texture3D.MipLevels			= MipLevels;
	Desc.Texture3D.ResourceMinLODClamp	= 0.0f;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12ShaderResourceView(Device.Get(), Resource, Desc);
	}
}

// UnorderedAccessView
UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Buffer* Buffer, 
	Uint64 FirstElement, 
	Uint32 NumElements, 
	EFormat Format, 
	Uint64 CounterOffsetInBytes) const
{
	VALIDATE(Buffer != nullptr);
	VALIDATE(Buffer->HasUnorderedAccessUsage());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension					= D3D12_UAV_DIMENSION_BUFFER;
	Desc.Format							= ConvertFormat(Format);
	Desc.Buffer.FirstElement			= FirstElement;
	Desc.Buffer.NumElements				= NumElements;
	Desc.Buffer.Flags					= D3D12_BUFFER_UAV_FLAG_RAW;
	Desc.Buffer.StructureByteStride		= 0;
	Desc.Buffer.CounterOffsetInBytes	= CounterOffsetInBytes;

	// Make sure we actually have a buffer
	const D3D12Resource* Resource = D3D12BufferCast(Buffer);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified Buffer was not of buffer-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Buffer* Buffer,
	Uint64 FirstElement,
	Uint32 NumElements,
	Uint32 Stride,
	Uint64 CounterOffsetInBytes) const
{
	VALIDATE(Buffer != nullptr);
	VALIDATE(Buffer->HasUnorderedAccessUsage());
	VALIDATE(Stride != 0);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension					= D3D12_UAV_DIMENSION_BUFFER;
	Desc.Format							= DXGI_FORMAT_UNKNOWN;
	Desc.Buffer.FirstElement			= FirstElement;
	Desc.Buffer.NumElements				= NumElements;
	Desc.Buffer.Flags					= D3D12_BUFFER_UAV_FLAG_NONE;
	Desc.Buffer.StructureByteStride		= Stride;
	Desc.Buffer.CounterOffsetInBytes	= CounterOffsetInBytes;

	// Make sure we actually have a buffer
	const D3D12Resource* Resource = D3D12BufferCast(Buffer);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified Buffer was not of buffer-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Texture1D* Texture, 
	EFormat Format, 
	Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension		= D3D12_UAV_DIMENSION_TEXTURE1D;
	Desc.Format				= ConvertFormat(Format);
	Desc.Texture1D.MipSlice	= MipSlice;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Texture1DArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture1DArray.MipSlice		= MipSlice;
	Desc.Texture1DArray.ArraySize		= ArraySize;
	Desc.Texture1DArray.FirstArraySlice = FirstArraySlice;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Texture2D* Texture, 
	EFormat Format, 
	Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(Texture->IsMultiSampled() == false);
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
	Desc.Format					= ConvertFormat(Format);
	Desc.Texture2D.MipSlice		= MipSlice;
	Desc.Texture2D.PlaneSlice	= 0;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Texture2DArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(Texture->IsMultiSampled() == false);
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture2DArray.MipSlice		= MipSlice;
	Desc.Texture2DArray.ArraySize		= ArraySize;
	Desc.Texture2DArray.FirstArraySlice	= FirstArraySlice;
	Desc.Texture2DArray.PlaneSlice		= 0;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(Texture->IsMultiSampled() == false);
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;
	Desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture2DArray.MipSlice		= MipSlice;
	Desc.Texture2DArray.ArraySize		= TEXTURE_CUBE_FACE_COUNT;
	Desc.Texture2DArray.FirstArraySlice	= 0;
	Desc.Texture2DArray.PlaneSlice		= 0;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const TextureCubeArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 ArraySlice) const
{
	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;

	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(Texture->IsMultiSampled() == false);
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(ArraySlice < (Texture->GetArrayCount() / TEXTURE_CUBE_FACE_COUNT));
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;
	Desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture2DArray.MipSlice		= MipSlice;
	Desc.Texture2DArray.ArraySize		= TEXTURE_CUBE_FACE_COUNT;
	Desc.Texture2DArray.FirstArraySlice	= ArraySlice * TEXTURE_CUBE_FACE_COUNT;
	Desc.Texture2DArray.PlaneSlice		= 0;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(
	const Texture3D* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstDepthSlice, 
	Uint32 DepthSlices) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstDepthSlice + DepthSlices < Texture->GetDepth());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	Desc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE3D;
	Desc.Format					= ConvertFormat(Format);
	Desc.Texture3D.MipSlice		= MipSlice;
	Desc.Texture3D.FirstWSlice	= FirstDepthSlice;
	Desc.Texture3D.WSize		= DepthSlices;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12UnorderedAccessView(Device.Get(), nullptr, Resource, Desc);
	}
}

// RenderTargetView
RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.ViewDimension		= D3D12_RTV_DIMENSION_TEXTURE1D;
	Desc.Format				= ConvertFormat(Format);
	Desc.Texture1D.MipSlice	= MipSlice;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const Texture1DArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture1DArray.MipSlice		= MipSlice;
	Desc.Texture1DArray.ArraySize		= ArraySize;
	Desc.Texture1DArray.FirstArraySlice	= FirstArraySlice;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
		Desc.Texture2D.MipSlice		= MipSlice;
		Desc.Texture2D.PlaneSlice	= 0;
	}
	else
	{
		Desc.ViewDimension	= D3D12_RTV_DIMENSION_TEXTURE2DMS;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const Texture2DArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.PlaneSlice		= 0;
		Desc.Texture2DArray.FirstArraySlice	= FirstArraySlice;
		Desc.Texture2DArray.ArraySize		= ArraySize;
	}
	else
	{
		Desc.ViewDimension						= D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.FirstArraySlice	= FirstArraySlice;
		Desc.Texture2DMSArray.ArraySize			= ArraySize;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MipSlice,
	Uint32 FirstFace,
	Uint32 FaceCount) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstFace + FaceCount < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.PlaneSlice		= 0;
		Desc.Texture2DArray.FirstArraySlice	= FirstFace;
		Desc.Texture2DArray.ArraySize		= FaceCount;
	}
	else
	{
		Desc.ViewDimension						= D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.FirstArraySlice	= FirstFace;
		Desc.Texture2DMSArray.ArraySize			= FaceCount;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const TextureCubeArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 ArraySlice, 
	Uint32 FirstFace, 
	Uint32 FaceCount) const
{
	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;

	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE((ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FirstFace + FaceCount < Texture->GetArrayCount());
	VALIDATE(FaceCount < TEXTURE_CUBE_FACE_COUNT);
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.PlaneSlice		= 0;
		Desc.Texture2DArray.FirstArraySlice = (TEXTURE_CUBE_FACE_COUNT * ArraySlice) + FirstFace;
		Desc.Texture2DArray.ArraySize		= FaceCount;
	}
	else
	{
		Desc.ViewDimension						= D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.FirstArraySlice	= (TEXTURE_CUBE_FACE_COUNT * ArraySlice) + FirstFace;
		Desc.Texture2DMSArray.ArraySize			= FaceCount;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const Texture3D* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstDepthSlice, 
	Uint32 DepthSlices) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstDepthSlice + DepthSlices < Texture->GetDepth());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.ViewDimension				= D3D12_RTV_DIMENSION_TEXTURE3D;
	Desc.Format						= ConvertFormat(Format);
	Desc.Texture3D.MipSlice			= MipSlice;
	Desc.Texture3D.FirstWSlice		= FirstDepthSlice;
	Desc.Texture3D.WSize			= DepthSlices;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12RenderTargetView(Device.Get(), Resource, Desc);
	}
}

// DepthStencilView
DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE1D;
	Desc.Format				= ConvertFormat(Format);
	Desc.Texture1D.MipSlice = MipSlice;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12DepthStencilView(Device.Get(), Resource, Desc);
	}
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(
	const Texture1DArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture1DArray.MipSlice		= MipSlice;
	Desc.Texture1DArray.ArraySize		= ArraySize;
	Desc.Texture1DArray.FirstArraySlice	= FirstArraySlice;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12DepthStencilView(Device.Get(), Resource, Desc);
	}
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;
		Desc.Texture2D.MipSlice	= MipSlice;
	}
	else
	{
		Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12DepthStencilView(Device.Get(), Resource, Desc);
	}
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(
	const Texture2DArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 FirstArraySlice, 
	Uint32 ArraySize) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstArraySlice + ArraySize < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.ArraySize		= ArraySize;
		Desc.Texture2DArray.FirstArraySlice	= FirstArraySlice;
	}
	else
	{
		Desc.ViewDimension						= D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.ArraySize			= ArraySize;
		Desc.Texture2DMSArray.FirstArraySlice	= FirstArraySlice;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12DepthStencilView(Device.Get(), Resource, Desc);
	}
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MipSlice,
	Uint32 FirstFace,
	Uint32 FaceCount) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FirstFace + FaceCount < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.ArraySize		= FaceCount;
		Desc.Texture2DArray.FirstArraySlice = FirstFace;
	}
	else
	{
		Desc.ViewDimension						= D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.ArraySize			= FaceCount;
		Desc.Texture2DMSArray.FirstArraySlice	= FirstFace;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12DepthStencilView(Device.Get(), Resource, Desc);
	}
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(
	const TextureCubeArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 ArraySlice, 
	Uint32 FirstFace,
	Uint32 FaceCount) const
{
	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;

	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE((ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FirstFace + FaceCount < Texture->GetArrayCount());
	VALIDATE(FaceCount < TEXTURE_CUBE_FACE_COUNT);
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.ArraySize		= FaceCount;
		Desc.Texture2DArray.FirstArraySlice = (ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FirstFace;
	}
	else
	{
		Desc.ViewDimension						= D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.ArraySize			= FaceCount;
		Desc.Texture2DMSArray.FirstArraySlice	= (ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FirstFace;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	if (!Resource)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Specified texture was not of texture-type");
		return nullptr;
	}
	else
	{
		return new D3D12DepthStencilView(Device.Get(), Resource, Desc);
	}
}

/*
* D3D12RenderingAPI - Pipeline
*/

Shader* D3D12RenderingAPI::CreateShader() const
{
	return nullptr;
}

DepthStencilState* D3D12RenderingAPI::CreateDepthStencilState() const
{
	return nullptr;
}

RasterizerState* D3D12RenderingAPI::CreateRasterizerState() const
{
	return nullptr;
}

BlendState* D3D12RenderingAPI::CreateBlendState() const
{
	return nullptr;
}

InputLayout* D3D12RenderingAPI::CreateInputLayout() const
{
	return nullptr;
}

GraphicsPipelineState* D3D12RenderingAPI::CreateGraphicsPipelineState() const
{
	return nullptr;
}

ComputePipelineState* D3D12RenderingAPI::CreateComputePipelineState() const
{
	return nullptr;
}

RayTracingPipelineState* D3D12RenderingAPI::CreateRayTracingPipelineState() const
{
	return nullptr;
}

bool D3D12RenderingAPI::IsRayTracingSupported() const
{
	return Device->IsRayTracingSupported();
}

bool D3D12RenderingAPI::UAVSupportsFormat(EFormat Format) const
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
	Memory::Memzero(&FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

	HRESULT Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
	if (SUCCEEDED(Result))
	{
		if (FeatureData.TypedUAVLoadAdditionalFormats)
		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport =
			{
				ConvertFormat(Format),
				D3D12_FORMAT_SUPPORT1_NONE,
				D3D12_FORMAT_SUPPORT2_NONE
			};

			Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
			if (FAILED(Result) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
			{
				return false;
			}
		}
	}

	return true;
}

bool D3D12RenderingAPI::AllocateBuffer(
	D3D12Resource& Resource, 
	D3D12_HEAP_TYPE HeapType, 
	D3D12_RESOURCE_STATES InitalState, 
	D3D12_RESOURCE_FLAGS Flags, 
	Uint32 SizeInBytes) const
{
	D3D12_HEAP_PROPERTIES HeapProperties;
	Memory::Memzero(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));

	HeapProperties.Type					= HeapType;
	HeapProperties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags					= Flags;
	Desc.Format					= DXGI_FORMAT_UNKNOWN;
	Desc.Width					= SizeInBytes;
	Desc.Height					= 1;
	Desc.DepthOrArraySize		= 1;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= 1;
	Desc.SampleDesc.Count		= 1;
	Desc.SampleDesc.Quality		= 0;

	HRESULT HR = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		InitalState,
		nullptr, 
		IID_PPV_ARGS(&Resource.D3DResource));
	if (SUCCEEDED(HR))
	{
		Resource.Address		= Resource.D3DResource->GetGPUVirtualAddress();
		Resource.Desc			= Desc;
		Resource.HeapType		= HeapType;
		Resource.ResourceState	= InitalState;
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create resource");
		return false;
	}
}

bool D3D12RenderingAPI::AllocateTexture(
	D3D12Resource& Resource, 
	D3D12_HEAP_TYPE HeapType, 
	D3D12_RESOURCE_STATES InitalState, 
	const D3D12_RESOURCE_DESC& Desc) const
{
	D3D12_HEAP_PROPERTIES HeapProperties;
	Memory::Memzero(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));

	HeapProperties.Type					= HeapType;
	HeapProperties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;

	HRESULT HR = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		InitalState, 
		nullptr, 
		IID_PPV_ARGS(&Resource.D3DResource));
	if (SUCCEEDED(HR))
	{
		Resource.Address	= Resource.D3DResource->GetGPUVirtualAddress();
		Resource.Desc		= Desc;
		Resource.HeapType	= HeapType;
		Resource.ResourceState = InitalState;
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create resource");
		return false;
	}
}

bool D3D12RenderingAPI::UploadResource(D3D12Resource& Resource, const ResourceData* InitalData) const
{
	return false;
}