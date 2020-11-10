#include "Containers/TUniquePtr.h"

#include "D3D12RenderingAPI.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12PipelineState.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RayTracingPipelineState.h"
#include "D3D12RayTracingScene.h"
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12SwapChain.h"
#include "D3D12Helpers.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Shader.h"

#include <d3d11.h>
#include <algorithm>

/*
* D3D12RenderingAPI
*/

D3D12RenderingAPI::D3D12RenderingAPI()
	: GenericRenderingAPI(ERenderingAPI::RenderingAPI_D3D12)
	, SwapChain(nullptr)
	, Device(nullptr)
	, DirectCmdQueue(nullptr)
	, DirectCmdContext(nullptr)
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
	
	// Create commandqueue
	DirectCmdQueue = TSharedPtr(Device->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	if (!DirectCmdQueue)
	{
		return false;
	}

	// Create default rootsignatures
	if (!DefaultRootSignatures.Init(Device.Get()))
	{
		return false;
	}

	// Create commandcontext
	DirectCmdContext = MakeShared<D3D12CommandContext>(Device.Get(), DirectCmdQueue.Get(), DefaultRootSignatures);
	if (!DirectCmdContext->Initialize())
	{
		return false;
	}

	// Create swapchain
	SwapChain = Device->CreateSwapChain(StaticCast<WindowsWindow>(RenderWindow).Get(), DirectCmdQueue.Get());
	if (!SwapChain)
	{
		return false;
	}

	// TODO: Create RenderTargets

	return true;
}

/*
* Textures
*/

Texture1D* D3D12RenderingAPI::CreateTexture1D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 MipLevels, 
	const ClearValue& OptimizedClearValue) const
{
	return CreateTextureResource<D3D12Texture1D>(
		InitalData,
		Format,
		Usage,
		Width,
		MipLevels,
		OptimizedClearValue);
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
	return CreateTextureResource<D3D12Texture1DArray>(
		InitalData,
		Format, 
		Usage, 
		Width, 
		MipLevels, 
		ArrayCount, 
		OptimizedClearValue);
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
	return CreateTextureResource<D3D12Texture2D>(
		InitalData,
		Format, 
		Usage, 
		Width, 
		Height, 
		MipLevels, 
		SampleCount, 
		OptimizedClearValue);
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
	return CreateTextureResource<D3D12Texture2DArray>(
		InitalData,
		Format,
		Usage,
		Width,
		Height,
		MipLevels,
		ArrayCount,
		SampleCount,
		OptimizedClearValue);
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
	return CreateTextureResource<D3D12TextureCube>(
		InitalData,
		Format,
		Usage,
		Size,
		MipLevels,
		SampleCount,
		OptimizedClearValue);
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
	return CreateTextureResource<D3D12TextureCubeArray>(
		InitalData,
		Format,
		Usage,
		Size,
		MipLevels,
		ArrayCount,
		SampleCount,
		OptimizedClearValue);
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
	return CreateTextureResource<D3D12Texture3D>(
		InitalData,
		Format,
		Usage,
		Width,
		Height,
		Depth,
		MipLevels,
		OptimizedClearValue);
}

/*
* Buffers
*/

VertexBuffer* D3D12RenderingAPI::CreateVertexBuffer(
	const ResourceData* InitalData, 
	Uint32 SizeInBytes, 
	Uint32 StrideInBytes, 
	Uint32 Usage) const
{
	D3D12VertexBuffer* NewBuffer = CreateBufferResource<D3D12VertexBuffer>(InitalData, SizeInBytes, StrideInBytes, Usage);
	if (!NewBuffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create VertexBuffer");
		return nullptr;
	}

	D3D12_VERTEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	View.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	View.SizeInBytes	= SizeInBytes;
	View.StrideInBytes	= StrideInBytes;

	NewBuffer->VertexBufferView = View;
	return NewBuffer;
}

IndexBuffer* D3D12RenderingAPI::CreateIndexBuffer(
	const ResourceData* InitalData, 
	Uint32 SizeInBytes, 
	EIndexFormat IndexFormat, 
	Uint32 Usage) const
{
	D3D12IndexBuffer* NewBuffer = CreateBufferResource<D3D12IndexBuffer>(InitalData, SizeInBytes, IndexFormat, Usage);
	if (!NewBuffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create IndexBuffer");
		return nullptr;
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
	D3D12ConstantBuffer* NewBuffer = CreateBufferResource<D3D12ConstantBuffer>(InitalData, SizeInBytes, Usage);
	if (!NewBuffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create ConstantBuffer");
		return nullptr;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
	Memory::Memzero(&ViewDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));

	ViewDesc.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes	= NewBuffer->GetAllocatedSize();

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
	D3D12StructuredBuffer* NewBuffer = CreateBufferResource<D3D12StructuredBuffer>(InitalData, SizeInBytes, Stride, Usage);
	if (!NewBuffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create StructuredBuffer");
		return nullptr;
	}
	else
	{
		return NewBuffer;
	}
}

/*
* RayTracing
*/

RayTracingGeometry* D3D12RenderingAPI::CreateRayTracingGeometry() const
{
	return nullptr;
}

RayTracingScene* D3D12RenderingAPI::CreateRayTracingScene() const
{
	return nullptr;
}

/*
* ShaderResourceView
*/

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
	return CreateShaderResourceView(Resource, Desc);
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
	return CreateShaderResourceView(Resource, Desc);
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture1D* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
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
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture2D* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
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
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
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
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture3D* Texture, 
	EFormat Format, 
	Uint32 MostDetailedMip, 
	Uint32 MipLevels) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasShaderResourceUsage());
	VALIDATE(MostDetailedMip + MipLevels <= Texture->GetMipLevels());
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
	return CreateShaderResourceView(Resource, Desc);
}

/*
* UnorderedAccessView
*/

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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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

	Desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format							= ConvertFormat(Format);
	Desc.Texture2DArray.MipSlice		= MipSlice;
	Desc.Texture2DArray.ArraySize		= TEXTURE_CUBE_FACE_COUNT;
	Desc.Texture2DArray.FirstArraySlice	= ArraySlice * TEXTURE_CUBE_FACE_COUNT;
	Desc.Texture2DArray.PlaneSlice		= 0;

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
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
	return CreateUnorderedAccessView(nullptr, Resource, Desc);
}

/*
* RenderTargetView
*/

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
	return CreateRenderTargetView(Resource, Desc);
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
	return CreateRenderTargetView(Resource, Desc);
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
	return CreateRenderTargetView(Resource, Desc);
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
	return CreateRenderTargetView(Resource, Desc);
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MipSlice,
	Uint32 FaceIndex) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FaceIndex < Texture->GetArrayCount());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.PlaneSlice		= 0;
		Desc.Texture2DArray.FirstArraySlice	= FaceIndex;
		Desc.Texture2DArray.ArraySize		= 1;
	}
	else
	{
		Desc.ViewDimension						= D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.FirstArraySlice	= FaceIndex;
		Desc.Texture2DMSArray.ArraySize			= 1;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	return CreateRenderTargetView(Resource, Desc);
}

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(
	const TextureCubeArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 ArraySlice, 
	Uint32 FaceIndex) const
{
	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;

	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasRenderTargetUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE((ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FaceIndex < Texture->GetArrayCount());
	VALIDATE(FaceIndex < TEXTURE_CUBE_FACE_COUNT);
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_RENDER_TARGET_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.PlaneSlice		= 0;
		Desc.Texture2DArray.FirstArraySlice = (TEXTURE_CUBE_FACE_COUNT * ArraySlice) + FaceIndex;
		Desc.Texture2DArray.ArraySize		= 1;
	}
	else
	{
		Desc.ViewDimension						= D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.FirstArraySlice	= (TEXTURE_CUBE_FACE_COUNT * ArraySlice) + FaceIndex;
		Desc.Texture2DMSArray.ArraySize			= 1;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	return CreateRenderTargetView(Resource, Desc);
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
	return CreateRenderTargetView(Resource, Desc);
}

/*
* DepthStencilView
*/

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
	return CreateDepthStencilView(Resource, Desc);
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
	return CreateDepthStencilView(Resource, Desc);
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
	return CreateDepthStencilView(Resource, Desc);
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
	return CreateDepthStencilView(Resource, Desc);
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(
	const TextureCube* Texture, 
	EFormat Format, 
	Uint32 MipSlice,
	Uint32 FaceIndex) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(FaceIndex < 6);
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.ArraySize		= 1;
		Desc.Texture2DArray.FirstArraySlice = FaceIndex;
	}
	else
	{
		Desc.ViewDimension						= D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.ArraySize			= 1;
		Desc.Texture2DMSArray.FirstArraySlice	= FaceIndex;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	return CreateDepthStencilView(Resource, Desc);
}

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(
	const TextureCubeArray* Texture, 
	EFormat Format, 
	Uint32 MipSlice, 
	Uint32 ArraySlice, 
	Uint32 FaceIndex) const
{
	constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;

	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasDepthStencilUsage());
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE((ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FaceIndex < Texture->GetArrayCount());
	VALIDATE(FaceIndex < TEXTURE_CUBE_FACE_COUNT);
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

	Desc.Format = ConvertFormat(Format);
	if (!Texture->IsMultiSampled())
	{
		Desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		Desc.Texture2DArray.MipSlice		= MipSlice;
		Desc.Texture2DArray.ArraySize		= 1;
		Desc.Texture2DArray.FirstArraySlice = (ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FaceIndex;
	}
	else
	{
		Desc.ViewDimension						= D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		Desc.Texture2DMSArray.ArraySize			= 1;
		Desc.Texture2DMSArray.FirstArraySlice	= (ArraySlice * TEXTURE_CUBE_FACE_COUNT) + FaceIndex;
	}

	// Make sure that the texture actually is a textures
	const D3D12Resource* Resource = D3D12TextureCast(Texture);
	return CreateDepthStencilView(Resource, Desc);
}

/*
* Pipeline
*/

ComputeShader* D3D12RenderingAPI::CreateComputeShader(const TArray<Uint8>& ShaderCode) const
{
	D3D12ComputeShader* Shader = new D3D12ComputeShader(Device.Get(), ShaderCode);
	Shader->CreateRootSignature();
	return Shader;
}

VertexShader* D3D12RenderingAPI::CreateVertexShader(const TArray<Uint8>& ShaderCode) const
{
	return new D3D12VertexShader(Device.Get(), ShaderCode);
}

HullShader* D3D12RenderingAPI::CreateHullShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

DomainShader* D3D12RenderingAPI::CreateDomainShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

GeometryShader* D3D12RenderingAPI::CreateGeometryShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

MeshShader* D3D12RenderingAPI::CreateMeshShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

AmplificationShader* D3D12RenderingAPI::CreateAmplificationShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

PixelShader* D3D12RenderingAPI::CreatePixelShader(const TArray<Uint8>& ShaderCode) const
{
	return new D3D12PixelShader(Device.Get(), ShaderCode);
}

RayGenShader* D3D12RenderingAPI::CreateRayGenShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

RayHitShader* D3D12RenderingAPI::CreateRayHitShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

RayMissShader* D3D12RenderingAPI::CreateRayMissShader(const TArray<Uint8>& ShaderCode) const
{
	return nullptr;
}

DepthStencilState* D3D12RenderingAPI::CreateDepthStencilState(
	const DepthStencilStateCreateInfo& CreateInfo) const
{
	D3D12_DEPTH_STENCIL_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	
	Desc.DepthEnable		= CreateInfo.DepthEnable;
	Desc.DepthFunc			= ConvertComparisonFunc(CreateInfo.DepthFunc);
	Desc.DepthWriteMask		= ConvertDepthWriteMask(CreateInfo.DepthWriteMask);
	Desc.StencilEnable		= CreateInfo.StencilEnable;
	Desc.StencilReadMask	= CreateInfo.StencilReadMask;
	Desc.StencilWriteMask	= CreateInfo.StencilWriteMask;
	Desc.FrontFace			= ConvertDepthStencilOp(CreateInfo.FrontFace);
	Desc.BackFace			= ConvertDepthStencilOp(CreateInfo.BackFace);

	return new D3D12DepthStencilState(Device.Get(), Desc);
}

RasterizerState* D3D12RenderingAPI::CreateRasterizerState(
	const RasterizerStateCreateInfo& CreateInfo) const
{
	D3D12_RASTERIZER_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RASTERIZER_DESC));

	Desc.AntialiasedLineEnable	= CreateInfo.AntialiasedLineEnable;
	Desc.ConservativeRaster =
		(CreateInfo.EnableConservativeRaster) ?
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON :
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	Desc.CullMode				= ConvertCullMode(CreateInfo.CullMode);
	Desc.DepthBias				= CreateInfo.DepthBias;
	Desc.DepthBiasClamp			= CreateInfo.DepthBiasClamp;
	Desc.DepthClipEnable		= CreateInfo.DepthClipEnable;
	Desc.SlopeScaledDepthBias	= CreateInfo.SlopeScaledDepthBias;
	Desc.FillMode				= ConvertFillMode(CreateInfo.FillMode);
	Desc.ForcedSampleCount		= CreateInfo.ForcedSampleCount;
	Desc.FrontCounterClockwise	= CreateInfo.FrontCounterClockwise;
	Desc.MultisampleEnable		= CreateInfo.MultisampleEnable;

	return new D3D12RasterizerState(Device.Get(), Desc);
}

BlendState* D3D12RenderingAPI::CreateBlendState(
	const BlendStateCreateInfo& CreateInfo) const
{
	D3D12_BLEND_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_BLEND_DESC));

	Desc.AlphaToCoverageEnable	= CreateInfo.AlphaToCoverageEnable;
	Desc.IndependentBlendEnable	= CreateInfo.IndependentBlendEnable;	
	for (Uint32 i = 0; i < 8; i++)
	{
		Desc.RenderTarget[i].BlendEnable			= CreateInfo.RenderTarget[i].BlendEnable;
		Desc.RenderTarget[i].BlendOp				= ConvertBlendOp(CreateInfo.RenderTarget[i].BlendOp);
		Desc.RenderTarget[i].BlendOpAlpha			= ConvertBlendOp(CreateInfo.RenderTarget[i].BlendOpAlpha);
		Desc.RenderTarget[i].DestBlend				= ConvertBlend(CreateInfo.RenderTarget[i].DestBlend);
		Desc.RenderTarget[i].DestBlendAlpha			= ConvertBlend(CreateInfo.RenderTarget[i].DestBlendAlpha);
		Desc.RenderTarget[i].SrcBlend				= ConvertBlend(CreateInfo.RenderTarget[i].SrcBlend);
		Desc.RenderTarget[i].SrcBlendAlpha			= ConvertBlend(CreateInfo.RenderTarget[i].SrcBlendAlpha);
		Desc.RenderTarget[i].LogicOpEnable			= CreateInfo.RenderTarget[i].LogicOpEnable;
		Desc.RenderTarget[i].LogicOp				= ConvertLogicOp(CreateInfo.RenderTarget[i].LogicOp);
		Desc.RenderTarget[i].RenderTargetWriteMask	= ConvertRenderTargetWriteState(CreateInfo.RenderTarget[i].RenderTargetWriteMask);
	}

	return new D3D12BlendState(Device.Get(), Desc);
}

InputLayoutState* D3D12RenderingAPI::CreateInputLayout(
	const InputLayoutStateCreateInfo& CreateInfo) const
{
	return new D3D12InputLayoutState(Device.Get(), CreateInfo);
}

GraphicsPipelineState* D3D12RenderingAPI::CreateGraphicsPipelineState(
	const GraphicsPipelineStateCreateInfo& CreateInfo) const
{
	using namespace Microsoft::WRL;

	struct alignas(void*) GraphicsPipelineStream
	{
		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
			ID3D12RootSignature* RootSignature = nullptr;
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE	Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
			D3D12_INPUT_LAYOUT_DESC InputLayout = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
			D3D12_SHADER_BYTECODE VertexShader = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
			D3D12_SHADER_BYTECODE PixelShader = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
			D3D12_RT_FORMAT_ARRAY RenderTargetInfo = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
			DXGI_FORMAT DepthBufferFormat = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
			D3D12_RASTERIZER_DESC RasterizerDesc = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
			D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
			D3D12_BLEND_DESC BlendStateDesc = { };
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type10 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
			DXGI_SAMPLE_DESC SampleDesc = { };
		};
	} PipelineStream;

	// InputLayout
	D3D12_INPUT_LAYOUT_DESC& InputLayoutDesc = PipelineStream.InputLayout;
	
	D3D12InputLayoutState* DxInputLayoutState = static_cast<D3D12InputLayoutState*>(CreateInfo.InputLayoutState);
	if (!DxInputLayoutState)
	{
		InputLayoutDesc.pInputElementDescs	= nullptr;
		InputLayoutDesc.NumElements			= 0;
	}
	else
	{
		InputLayoutDesc = DxInputLayoutState->GetDesc();
	}

	// VertexShader
	D3D12VertexShader* DxVertexShader = static_cast<D3D12VertexShader*>(CreateInfo.ShaderState.VertexShader);
	VALIDATE(DxVertexShader != nullptr);

	D3D12_SHADER_BYTECODE& VertexShader = PipelineStream.VertexShader;
	VertexShader = DxVertexShader->GetShaderByteCode();

	// PixelShader
	D3D12PixelShader* DxPixelShader = static_cast<D3D12PixelShader*>(CreateInfo.ShaderState.PixelShader);
	
	D3D12_SHADER_BYTECODE& PixelShader = PipelineStream.PixelShader;
	if (!DxPixelShader)
	{
		PixelShader.pShaderBytecode	= nullptr;
		PixelShader.BytecodeLength	= 0;
	}
	else
	{
		PixelShader = DxPixelShader->GetShaderByteCode();
	}

	// RenderTarget
	D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = PipelineStream.RenderTargetInfo;

	const Uint32 NumRenderTargets = CreateInfo.PipelineFormats.NumRenderTargets;
	for (Uint32 Index = 0; Index < NumRenderTargets; Index++)
	{
		RenderTargetInfo.RTFormats[Index] = ConvertFormat(CreateInfo.PipelineFormats.RenderTargetFormats[Index]);
	}
	RenderTargetInfo.NumRenderTargets = NumRenderTargets;

	// DepthStencil
	PipelineStream.DepthBufferFormat = ConvertFormat(CreateInfo.PipelineFormats.DepthStencilFormat);

	// RasterizerState
	D3D12RasterizerState* DxRasterizerState = static_cast<D3D12RasterizerState*>(CreateInfo.RasterizerState);
	VALIDATE(DxRasterizerState != nullptr);

	D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
	RasterizerDesc = DxRasterizerState->GetDesc();

	// DepthStencilState
	D3D12DepthStencilState* DxDepthStencilState = static_cast<D3D12DepthStencilState*>(CreateInfo.DepthStencilState);
	VALIDATE(DxDepthStencilState != nullptr);

	D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
	DepthStencilDesc = DxDepthStencilState->GetDesc();

	// BlendState
	D3D12BlendState* DxBlendState = static_cast<D3D12BlendState*>(CreateInfo.BlendState);
	VALIDATE(DxBlendState != nullptr);

	D3D12_BLEND_DESC& BlendStateDesc = PipelineStream.BlendStateDesc;
	BlendStateDesc = DxBlendState->GetDesc();

	// RootSignature
	VALIDATE(DefaultRootSignatures.Graphics != nullptr);
	PipelineStream.RootSignature = DefaultRootSignatures.Graphics->GetRootSignature();

	// Topology
	PipelineStream.PrimitiveTopologyType = ConvertPrimitiveTopologyType(CreateInfo.PrimitiveTopologyType);

	// MSAA
	DXGI_SAMPLE_DESC& SamplerDesc = PipelineStream.SampleDesc;
	SamplerDesc.Count	= CreateInfo.SampleCount;
	SamplerDesc.Quality	= CreateInfo.SampleQuality;

	// Create PipelineState
	D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
	Memory::Memzero(&PipelineStreamDesc, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));

	PipelineStreamDesc.pPipelineStateSubobjectStream	= &PipelineStream;
	PipelineStreamDesc.SizeInBytes						= sizeof(GraphicsPipelineStream);

	ComPtr<ID3D12PipelineState> PipelineState;
	HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
	if (SUCCEEDED(hResult))
	{
		D3D12GraphicsPipelineState* Pipeline = new D3D12GraphicsPipelineState(Device.Get());
		Pipeline->PipelineState = PipelineState;

		LOG_INFO("[D3D12RenderingAPI]: Created GraphicsPipelineState");
		return Pipeline;
	}
	else
	{
		LOG_ERROR("[D3D12RenderingAPI]: FAILED to Create GraphicsPipelineState");
		return nullptr;
	}
}

ComputePipelineState* D3D12RenderingAPI::CreateComputePipelineState(
	const ComputePipelineStateCreateInfo& Info) const
{
	using namespace Microsoft::WRL;

	struct alignas(void*) ComputePipelineStream
	{
		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
			ID3D12RootSignature* RootSignature = nullptr;
		};

		struct alignas(void*)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
			D3D12_SHADER_BYTECODE ComputeShader = { };
		};
	} PipelineStream;

	VALIDATE(Info.Shader != nullptr);

	// Shader
	D3D12ComputeShader& Shader = *static_cast<D3D12ComputeShader*>(Info.Shader);
	PipelineStream.ComputeShader = Shader.GetShaderByteCode();

	// Check if shader contains a rootsignature, or use the default one
	D3D12RootSignature* RootSignature = Shader.GetRootSignature();
	if (!RootSignature)
	{
		RootSignature = DefaultRootSignatures.Compute.Get();
	}
	
	PipelineStream.RootSignature = RootSignature->GetRootSignature();

	// Create PipelineState
	D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
	Memory::Memzero(&PipelineStreamDesc, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));
	
	PipelineStreamDesc.pPipelineStateSubobjectStream	= &PipelineStream;
	PipelineStreamDesc.SizeInBytes						= sizeof(ComputePipelineStream);

	ComPtr<ID3D12PipelineState> PipelineState;
	HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
	if (SUCCEEDED(hResult))
	{
		D3D12ComputePipelineState* Pipeline = new D3D12ComputePipelineState(Device.Get());
		Pipeline->PipelineState = PipelineState;

		LOG_INFO("[D3D12RenderingAPI]: Created ComputePipelineState");
		return Pipeline;
	}
	else
	{
		LOG_ERROR("[D3D12RenderingAPI]: FAILED to Create ComputePipelineState");
		return nullptr;
	}
}

RayTracingPipelineState* D3D12RenderingAPI::CreateRayTracingPipelineState() const
{
	return nullptr;
}

/*
* Supported features
*/

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

/*
* Allocate resources
*/

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

	HRESULT hr = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		InitalState,
		nullptr, 
		IID_PPV_ARGS(&Resource.D3DResource));
	if (SUCCEEDED(hr))
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

	HRESULT hr = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		InitalState, 
		nullptr, 
		IID_PPV_ARGS(&Resource.D3DResource));
	if (SUCCEEDED(hr))
	{
		Resource.Address	= NULL;
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


/*
* Resource uploading
*/

bool D3D12RenderingAPI::UploadBuffer(D3D12Buffer& Buffer, Uint32 SizeInBytes, const ResourceData* InitalData) const
{
	if (Buffer.GetHeapType() == D3D12_HEAP_TYPE_UPLOAD)
	{
		VALIDATE(Buffer.GetAllocatedSize() <= SizeInBytes);

		VoidPtr HostData =  Buffer.Map(nullptr);
		if (HostData)
		{
			Memory::Memcpy(HostData, InitalData->Data, SizeInBytes);
			Buffer.Unmap(nullptr);

			return true;
		}
	}

	// TODO: Handle non uploadheaps
	return false;
}

bool D3D12RenderingAPI::UploadTexture(D3D12Texture& Texture, const ResourceData* InitalData) const
{
	// TODO: Finish this function
	return false;
}