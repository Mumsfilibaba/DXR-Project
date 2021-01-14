#include "Containers/TUniquePtr.h"

#include "Windows/WindowsWindow.h"

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
#include "D3D12Helpers.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Shader.h"
#include "D3D12SamplerState.h"
#include "D3D12Viewport.h"

#include <algorithm>

/*
* D3D12RenderingAPI
*/

D3D12RenderingAPI::D3D12RenderingAPI()
	: GenericRenderingAPI(ERenderingAPI::RenderingAPI_D3D12)
	, Device(nullptr)
	, DirectCmdQueue(nullptr)
	, DirectCmdContext(nullptr)
{
}

D3D12RenderingAPI::~D3D12RenderingAPI()
{
	SAFEDELETE(Device);
}

Bool D3D12RenderingAPI::Init(Bool EnableDebug)
{
	// Create device
	Device = DBG_NEW D3D12Device(EnableDebug, EnableDebug ? true : false);
	if (!Device->Init())
	{
		return false;
	}
	
	DirectCmdQueue = Device->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	if (!DirectCmdQueue)
	{
		return false;
	}

	if (!DefaultRootSignatures.CreateRootSignatures(Device))
	{
		return false;
	}

	DirectCmdContext = DBG_NEW D3D12CommandContext(Device, DirectCmdQueue, DefaultRootSignatures);
	if (!DirectCmdContext->Init())
	{
		return false;
	}

	return true;
}

/*
* Textures
*/

Texture1D* D3D12RenderingAPI::CreateTexture1D(
	const ResourceData* InitalData, 
	EFormat Format, 
	UInt32 Usage, 
	UInt32 Width, 
	UInt32 MipLevels, 
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
	UInt32 Usage, 
	UInt32 Width, 
	UInt32 MipLevels, 
	UInt16 ArrayCount, 
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
	UInt32 Usage, 
	UInt32 Width, 
	UInt32 Height, 
	UInt32 MipLevels, 
	UInt32 SampleCount, 
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
	UInt32 Usage, 
	UInt32 Width, 
	UInt32 Height, 
	UInt32 MipLevels, 
	UInt16 ArrayCount,
	UInt32 SampleCount, 
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
	UInt32 Usage, 
	UInt32 Size, 
	UInt32 MipLevels, 
	UInt32 SampleCount, 
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
	UInt32 Usage, 
	UInt32 Size, 
	UInt32 MipLevels, 
	UInt16 ArrayCount,
	UInt32 SampleCount, 
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
	UInt32 Usage, 
	UInt32 Width, 
	UInt32 Height, 
	UInt16 Depth,
	UInt32 MipLevels, 
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
* Samplers
*/

SamplerState* D3D12RenderingAPI::CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo) const
{
	D3D12_SAMPLER_DESC Desc;
	Memory::Memzero(&Desc);

	Memory::Memcpy(Desc.BorderColor, CreateInfo.BorderColor, sizeof(Desc.BorderColor));
	Desc.AddressU		= ConvertSamplerMode(CreateInfo.AddressU);
	Desc.AddressV		= ConvertSamplerMode(CreateInfo.AddressV);
	Desc.AddressW		= ConvertSamplerMode(CreateInfo.AddressW);
	Desc.ComparisonFunc = ConvertComparisonFunc(CreateInfo.ComparisonFunc);
	Desc.Filter			= ConvertSamplerFilter(CreateInfo.Filter);
	Desc.MaxAnisotropy	= CreateInfo.MaxAnisotropy;
	Desc.MaxLOD			= CreateInfo.MaxLOD;
	Desc.MinLOD			= CreateInfo.MinLOD;
	Desc.MipLODBias		= CreateInfo.MipLODBias;

	return DBG_NEW D3D12SamplerState(Device, Device->GetGlobalSamplerDescriptorHeap(), Desc);
}

/*
* Buffers
*/

VertexBuffer* D3D12RenderingAPI::CreateVertexBuffer(
	const ResourceData* InitalData, 
	UInt32 SizeInBytes, 
	UInt32 StrideInBytes, 
	UInt32 Usage) const
{
	D3D12VertexBuffer* NewBuffer = CreateBufferResource<D3D12VertexBuffer>(
		InitalData,
		EResourceState::ResourceState_Common,
		SizeInBytes, 
		StrideInBytes, 
		Usage);
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
	UInt32 SizeInBytes, 
	EIndexFormat IndexFormat, 
	UInt32 Usage) const
{
	D3D12IndexBuffer* NewBuffer = CreateBufferResource<D3D12IndexBuffer>(
		InitalData, 
		EResourceState::ResourceState_Common, 
		SizeInBytes, 
		IndexFormat, 
		Usage);
	if (!NewBuffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create IndexBuffer");
		return nullptr;
	}

	D3D12_INDEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_INDEX_BUFFER_VIEW));

	View.BufferLocation = NewBuffer->GetGPUVirtualAddress();
	View.SizeInBytes	= SizeInBytes;
	if (IndexFormat == EIndexFormat::IndexFormat_UInt16)
	{
		View.Format	= DXGI_FORMAT_R16_UINT;
	}
	else if (IndexFormat == EIndexFormat::IndexFormat_UInt32)
	{
		View.Format = DXGI_FORMAT_R32_UINT;
	}

	NewBuffer->IndexBufferView = View;
	return NewBuffer;
}

ConstantBuffer* D3D12RenderingAPI::CreateConstantBuffer(
	const ResourceData* InitalData, 
	UInt32 SizeInBytes, 
	UInt32 Usage,
	EResourceState InitialState) const
{
	D3D12ConstantBuffer* NewBuffer = CreateBufferResource<D3D12ConstantBuffer>(
		InitalData, 
		InitialState,
		SizeInBytes, 
		Usage);
	if (!NewBuffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create ConstantBuffer");
		return nullptr;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
	Memory::Memzero(&ViewDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));

	ViewDesc.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes	= UInt32(NewBuffer->GetAllocatedSize());

	D3D12ConstantBufferView* View = DBG_NEW D3D12ConstantBufferView(Device, NewBuffer, ViewDesc);
	NewBuffer->View = View;
	return NewBuffer;
}

StructuredBuffer* D3D12RenderingAPI::CreateStructuredBuffer(
	const ResourceData* InitalData, 
	UInt32 SizeInBytes, 
	UInt32 Stride, 
	UInt32 Usage) const
{
	D3D12StructuredBuffer* NewBuffer = CreateBufferResource<D3D12StructuredBuffer>(
		InitalData, 
		EResourceState::ResourceState_Common,
		SizeInBytes, 
		Stride, 
		Usage);
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
	UInt32 FirstElement, 
	UInt32 ElementCount) const
{
	VALIDATE(Buffer != nullptr);
	VALIDATE(Buffer->HasShaderResourceUsage());

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	Desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.Buffer.FirstElement		= FirstElement;
	Desc.Buffer.NumElements			= ElementCount;
	Desc.Format						= DXGI_FORMAT_R32_TYPELESS;
	Desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	Desc.Buffer.StructureByteStride	= 0;

	const D3D12Resource* Resource = D3D12BufferCast(Buffer);
	return CreateShaderResourceView(Resource, Desc);
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Buffer* Buffer,
	UInt32 FirstElement,
	UInt32 ElementCount,
	UInt32 Stride) const
{
	VALIDATE(Buffer != nullptr);
	VALIDATE(Buffer->HasShaderResourceUsage());
	VALIDATE(Stride != 0);

	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	Desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	Desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Desc.Buffer.FirstElement		= FirstElement;
	Desc.Buffer.NumElements			= ElementCount;
	Desc.Format						= DXGI_FORMAT_UNKNOWN;
	Desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
	Desc.Buffer.StructureByteStride = Stride;

	const D3D12Resource* Resource = D3D12BufferCast(Buffer);
	return CreateShaderResourceView(Resource, Desc);
}

ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(
	const Texture1D* Texture, 
	EFormat Format, 
	UInt32 MostDetailedMip, 
	UInt32 MipLevels) const
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
	UInt32 MostDetailedMip, 
	UInt32 MipLevels,
	UInt32 FirstArraySlice,
	UInt32 ArraySize) const
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
	UInt32 MostDetailedMip, 
	UInt32 MipLevels) const
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
	UInt32 MostDetailedMip, 
	UInt32 MipLevels,
	UInt32 FirstArraySlice,
	UInt32 ArraySize) const
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
	UInt32 MostDetailedMip, 
	UInt32 MipLevels) const
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
	UInt32 MostDetailedMip, 
	UInt32 MipLevels, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
{
	constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;

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
	UInt32 MostDetailedMip, 
	UInt32 MipLevels) const
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
	UInt64 FirstElement, 
	UInt32 NumElements, 
	EFormat Format, 
	UInt64 CounterOffsetInBytes) const
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
	UInt64 FirstElement,
	UInt32 NumElements,
	UInt32 Stride,
	UInt64 CounterOffsetInBytes) const
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
	UInt32 MipSlice) const
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
	UInt32 MipSlice, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
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
	UInt32 MipSlice) const
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
	UInt32 MipSlice, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
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
	UInt32 MipSlice) const
{
	VALIDATE(Texture != nullptr);
	VALIDATE(Texture->HasUnorderedAccessUsage());
	VALIDATE(Texture->IsMultiSampled() == false);
	VALIDATE(MipSlice < Texture->GetMipLevels());
	VALIDATE(Format != EFormat::Format_Unknown);

	D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

	constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;
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
	UInt32 MipSlice, 
	UInt32 ArraySlice) const
{
	constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;

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
	UInt32 MipSlice, 
	UInt32 FirstDepthSlice, 
	UInt32 DepthSlices) const
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

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(const Texture1D* Texture, EFormat Format, UInt32 MipSlice) const
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
	UInt32 MipSlice, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
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

RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(const Texture2D* Texture, EFormat Format, UInt32 MipSlice) const
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
	UInt32 MipSlice, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
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
	UInt32 MipSlice,
	UInt32 FaceIndex) const
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
	UInt32 MipSlice, 
	UInt32 ArraySlice, 
	UInt32 FaceIndex) const
{
	constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;

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
	UInt32 MipSlice, 
	UInt32 FirstDepthSlice, 
	UInt32 DepthSlices) const
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

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(const Texture1D* Texture, EFormat Format, UInt32 MipSlice) const
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
	UInt32 MipSlice, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
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

DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(const Texture2D* Texture, EFormat Format, UInt32 MipSlice) const
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
	UInt32 MipSlice, 
	UInt32 FirstArraySlice, 
	UInt32 ArraySize) const
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
	UInt32 MipSlice,
	UInt32 FaceIndex) const
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
	UInt32 MipSlice, 
	UInt32 ArraySlice, 
	UInt32 FaceIndex) const
{
	constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;

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

ComputeShader* D3D12RenderingAPI::CreateComputeShader(const TArray<UInt8>& ShaderCode) const
{
	D3D12ComputeShader* Shader = DBG_NEW D3D12ComputeShader(Device, ShaderCode);
	Shader->CreateRootSignature();
	return Shader;
}

VertexShader* D3D12RenderingAPI::CreateVertexShader(const TArray<UInt8>& ShaderCode) const
{
	return DBG_NEW D3D12VertexShader(Device, ShaderCode);
}

HullShader* D3D12RenderingAPI::CreateHullShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

DomainShader* D3D12RenderingAPI::CreateDomainShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

GeometryShader* D3D12RenderingAPI::CreateGeometryShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

MeshShader* D3D12RenderingAPI::CreateMeshShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

AmplificationShader* D3D12RenderingAPI::CreateAmplificationShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

PixelShader* D3D12RenderingAPI::CreatePixelShader(const TArray<UInt8>& ShaderCode) const
{
	return DBG_NEW D3D12PixelShader(Device, ShaderCode);
}

RayGenShader* D3D12RenderingAPI::CreateRayGenShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

RayHitShader* D3D12RenderingAPI::CreateRayHitShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
	return nullptr;
}

RayMissShader* D3D12RenderingAPI::CreateRayMissShader(const TArray<UInt8>& ShaderCode) const
{
	// TODO: Finish this
	UNREFERENCED_VARIABLE(ShaderCode);
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

	return DBG_NEW D3D12DepthStencilState(Device, Desc);
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

	return DBG_NEW D3D12RasterizerState(Device, Desc);
}

BlendState* D3D12RenderingAPI::CreateBlendState(
	const BlendStateCreateInfo& CreateInfo) const
{
	D3D12_BLEND_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_BLEND_DESC));

	Desc.AlphaToCoverageEnable	= CreateInfo.AlphaToCoverageEnable;
	Desc.IndependentBlendEnable	= CreateInfo.IndependentBlendEnable;	
	for (UInt32 i = 0; i < 8; i++)
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

	return DBG_NEW D3D12BlendState(Device, Desc);
}

InputLayoutState* D3D12RenderingAPI::CreateInputLayout(
	const InputLayoutStateCreateInfo& CreateInfo) const
{
	return DBG_NEW D3D12InputLayoutState(Device, CreateInfo);
}

GraphicsPipelineState* D3D12RenderingAPI::CreateGraphicsPipelineState(
	const GraphicsPipelineStateCreateInfo& CreateInfo) const
{
	using namespace Microsoft::WRL;

	struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) GraphicsPipelineStream
	{
		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
			ID3D12RootSignature* RootSignature = nullptr;
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE	Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
			D3D12_INPUT_LAYOUT_DESC InputLayout = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
			D3D12_SHADER_BYTECODE VertexShader = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
			D3D12_SHADER_BYTECODE PixelShader = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
			D3D12_RT_FORMAT_ARRAY RenderTargetInfo = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
			DXGI_FORMAT DepthBufferFormat = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
			D3D12_RASTERIZER_DESC RasterizerDesc = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
			D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
			D3D12_BLEND_DESC BlendStateDesc = { };
		};

		struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
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

	const UInt32 NumRenderTargets = CreateInfo.PipelineFormats.NumRenderTargets;
	for (UInt32 Index = 0; Index < NumRenderTargets; Index++)
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

	D3D12RootSignature* RootSignature = DefaultRootSignatures.Graphics.Get();
	PipelineStream.RootSignature = RootSignature->GetRootSignature();

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
		D3D12GraphicsPipelineState* Pipeline = DBG_NEW D3D12GraphicsPipelineState(Device);
		Pipeline->PipelineState = PipelineState;

		// TODO: This should be refcounted
		Pipeline->RootSignature = RootSignature;

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
	VALIDATE(Info.Shader != nullptr);
	
	// Check if shader contains a rootsignature, or use the default one
	TSharedRef<D3D12ComputeShader> Shader			= MakeSharedRef<D3D12ComputeShader>(Info.Shader);
	TSharedRef<D3D12RootSignature> RootSignature	= MakeSharedRef<D3D12RootSignature>(Shader->GetRootSignature());
	if (!RootSignature)
	{
		RootSignature = DefaultRootSignatures.Compute;
	}

	D3D12ComputePipelineState* NewPipelineState = DBG_NEW D3D12ComputePipelineState(Device, Shader, RootSignature);
	if (NewPipelineState->Init())
	{
		return NewPipelineState;
	}
	else
	{
		return nullptr;
	}
}

RayTracingPipelineState* D3D12RenderingAPI::CreateRayTracingPipelineState() const
{
	return nullptr;
}

/*
* Viewport
*/

Viewport* D3D12RenderingAPI::CreateViewport(
	GenericWindow* Window,
	UInt32 Width,
	UInt32 Height,
	EFormat ColorFormat, 
	EFormat DepthFormat) const
{
	UNREFERENCED_VARIABLE(DepthFormat);

	// TODO: Take DepthFormat into account

	TSharedRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(Window);
	if (Width == 0)
	{
		Width = WinWindow->GetWidth();
	}

	if (Height == 0)
	{
		Height = WinWindow->GetHeight();
	}

	TSharedRef<D3D12Viewport> Viewport = DBG_NEW D3D12Viewport(
		Device, 
		DirectCmdContext.Get(), 
		WinWindow->GetHandle(), 
		Width,
		Height,
		ColorFormat);
	if (Viewport->Init())
	{
		return Viewport.ReleaseOwnership();
	}
	else
	{
		return nullptr;
	}
}

/*
* Supported features
*/

Bool D3D12RenderingAPI::IsRayTracingSupported() const
{
	return Device->IsRayTracingSupported();
}

Bool D3D12RenderingAPI::UAVSupportsFormat(EFormat Format) const
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

Bool D3D12RenderingAPI::AllocateBuffer(
	D3D12Resource& Resource, 
	D3D12_HEAP_TYPE HeapType, 
	D3D12_RESOURCE_STATES InitalState, 
	D3D12_RESOURCE_FLAGS Flags, 
	UInt32 SizeInBytes) const
{
	D3D12_HEAP_PROPERTIES HeapProperties;
	Memory::Memzero(&HeapProperties);

	HeapProperties.Type					= HeapType;
	HeapProperties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc);

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

	HRESULT Result = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		InitalState,
		nullptr, 
		IID_PPV_ARGS(&Resource.NativeResource));
	if (SUCCEEDED(Result))
	{
		Resource.Address		= Resource.NativeResource->GetGPUVirtualAddress();
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

Bool D3D12RenderingAPI::AllocateTexture(
	D3D12Resource& Resource, 
	D3D12_HEAP_TYPE HeapType, 
	D3D12_RESOURCE_STATES InitalState,
	D3D12_CLEAR_VALUE* OptimizedClearValue,
	const D3D12_RESOURCE_DESC& Desc) const
{
	D3D12_HEAP_PROPERTIES HeapProperties;
	Memory::Memzero(&HeapProperties);

	HeapProperties.Type					= HeapType;
	HeapProperties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;

	HRESULT Result = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		InitalState, 
		OptimizedClearValue,
		IID_PPV_ARGS(&Resource.NativeResource));
	if (SUCCEEDED(Result))
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

Bool D3D12RenderingAPI::UploadBuffer(Buffer& Buffer, UInt32 SizeInBytes, const ResourceData* InitalData) const
{
	if (Buffer.HasDynamicUsage())
	{
		VALIDATE(Buffer.GetSizeInBytes() <= SizeInBytes);

		Void* HostData = Buffer.Map(nullptr);
		if (HostData)
		{
			Memory::Memcpy(HostData, InitalData->Data, SizeInBytes);
			Buffer.Unmap(nullptr);

			return true;
		}
	}

	DirectCmdContext->Begin();

	DirectCmdContext->TransitionBuffer(
		&Buffer,
		EResourceState::ResourceState_Common,
		EResourceState::ResourceState_CopyDest);
	
	DirectCmdContext->UpdateBuffer(&Buffer, 0, SizeInBytes, InitalData->Data);

	DirectCmdContext->TransitionBuffer(
		&Buffer,
		EResourceState::ResourceState_CopyDest,
		EResourceState::ResourceState_Common);

	DirectCmdContext->End();

	return true;
}

Bool D3D12RenderingAPI::UploadTexture(Texture& Texture, const ResourceData* InitalData) const
{
	// TODO: Support other types than texture 2D
	Texture2D* Texture2D = Texture.AsTexture2D();
	if (!Texture2D)
	{
		return false;
	}

	DirectCmdContext->Begin();

	DirectCmdContext->TransitionTexture(
		Texture2D, 
		EResourceState::ResourceState_Common,
		EResourceState::ResourceState_CopyDest);

	const UInt32 Width 	= Texture.GetWidth();
	const UInt32 Height = Texture.GetHeight();
	DirectCmdContext->UpdateTexture2D(
		Texture2D,
		Width,
		Height,
		0,
		InitalData->Data);

	DirectCmdContext->TransitionTexture(
		Texture2D,
		EResourceState::ResourceState_CopyDest,
		EResourceState::ResourceState_Common);

	DirectCmdContext->End();

	return true;
}