#include "TextureFactory.h"

#include "Renderer.h"

#include "RenderingCore/RenderingAPI.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12CommandList.h"
#include "D3D12/D3D12CommandAllocator.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12Fence.h"
#include "D3D12/D3D12DescriptorHeap.h"
#include "D3D12/D3D12ComputePipelineState.h"
#include "D3D12/D3D12RootSignature.h"
#include "D3D12/D3D12ShaderCompiler.h"

#ifdef min
	#undef min
#endif

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct TextureFactoryData
{
	TUniquePtr<D3D12ComputePipelineState> PanoramaPSO;
	TUniquePtr<D3D12RootSignature> PanoramaRootSignature;
};

static TextureFactoryData GlobalFactoryData;

D3D12Texture* TextureFactory::LoadFromFile(const std::string& Filepath, uint32 CreateFlags, DXGI_FORMAT Format)
{
	int32 Width			= 0;
	int32 Height		= 0;
	int32 ChannelCount	= 0;

	// Load based on format
	TUniquePtr<byte> Pixels;
	if (Format == DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		Pixels = TUniquePtr<byte>(stbi_load(Filepath.c_str(), &Width, &Height, &ChannelCount, 4));
	}
	else if (Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
	{
		Pixels = TUniquePtr<byte>(reinterpret_cast<byte*>(stbi_loadf(Filepath.c_str(), &Width, &Height, &ChannelCount, 4)));
	}
	else
	{
		LOG_ERROR("[TextureFactory]: Format not supported");
		return nullptr;
	}

	// Check if succeeded
	if (!Pixels)
	{
		LOG_ERROR("[TextureFactory]: Failed to load image '" + Filepath + "'");
		return nullptr;
	}
	else
	{
		LOG_INFO("[TextureFactory]: Loaded image '" + Filepath + "'");
	}

	return LoadFromMemory(Pixels.Get(), Width, Height, CreateFlags, Format);
}

D3D12Texture* TextureFactory::LoadFromMemory(const byte* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, DXGI_FORMAT Format)
{
	if (Format != DXGI_FORMAT_R8G8B8A8_UNORM && Format != DXGI_FORMAT_R32G32B32A32_FLOAT)
	{
		LOG_ERROR("[TextureFactory]: Format not supported");
		return nullptr;
	}

	const bool GenerateMipLevels = CreateFlags & ETextureFactoryFlags::TEXTURE_FACTORY_FLAGS_GENERATE_MIPS;
	const uint32 MipLevels = GenerateMipLevels ? std::min<uint32>(uint32(std::log2<uint32>(Width)), uint32(std::log2<uint32>(Height))) : 1;

	VALIDATE(MipLevels != 0);

	// Create texture
	TextureProperties TextureProps = { };
	TextureProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	TextureProps.Width			= static_cast<uint16>(Width);
	TextureProps.Height			= static_cast<uint16>(Height);
	TextureProps.ArrayCount		= 1;
	TextureProps.MipLevels		= static_cast<uint16>(MipLevels);
	TextureProps.Format			= Format;
	TextureProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	TextureProps.MemoryType		= EMemoryType::MEMORY_TYPE_DEFAULT;
	TextureProps.SampleCount	= 1;

	TUniquePtr<D3D12Texture> Texture = TUniquePtr(RenderingAPI::Get().CreateTexture(TextureProps));
	if (!Texture)
	{
		return nullptr;
	}

	const uint32 Stride			= (Format == DXGI_FORMAT_R8G8B8A8_UNORM) ? 4 : 16;
	const uint32 RowPitch		= (Width * Stride + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
	const uint32 SizeInBytes	= Height * RowPitch;

	// ShaderResourceView
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format						= Format;
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Texture2D.MipLevels			= MipLevels;
	SrvDesc.Texture2D.MostDetailedMip	= 0;
	Texture->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(Texture->GetResource(), &SrvDesc)), 0);

	TSharedPtr<D3D12ImmediateCommandList> CommandList = RenderingAPI::StaticGetImmediateCommandList();
	CommandList->TransitionBarrier(Texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->UploadTextureData(Texture.Get(), Pixels, Format, Width, Height, 1, Stride, RowPitch);

	if (GenerateMipLevels)
	{
		CommandList->GenerateMips(Texture.Get());
	}

	CommandList->TransitionBarrier(Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CommandList->Flush();
	CommandList->WaitForCompletion();

	return Texture.Release();
}

D3D12Texture* TextureFactory::CreateTextureCubeFromPanorma(D3D12Texture* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, DXGI_FORMAT Format)
{
	VALIDATE(PanoramaSource->GetShaderResourceView(0));

	const bool GenerateMipLevels = CreateFlags & ETextureFactoryFlags::TEXTURE_FACTORY_FLAGS_GENERATE_MIPS;
	const uint16 MipLevels = (GenerateMipLevels) ? static_cast<uint16>(std::log2(CubeMapSize)) : 1U;

	// Create texture
	TextureProperties TextureProps = { };
	TextureProps.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	TextureProps.Width			= static_cast<uint16>(CubeMapSize);
	TextureProps.Height			= static_cast<uint16>(CubeMapSize);
	TextureProps.ArrayCount		= 6;
	TextureProps.MipLevels		= MipLevels;
	TextureProps.Format			= Format;
	TextureProps.MemoryType		= EMemoryType::MEMORY_TYPE_DEFAULT;
	TextureProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	TextureProps.SampleCount	= 1;

	TUniquePtr<D3D12Texture> StagingTexture = TUniquePtr(RenderingAPI::Get().CreateTexture(TextureProps));
	if (!StagingTexture)
	{
		return nullptr;
	}

	//Create UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = { };
	UAVDesc.Format							= Format;
	UAVDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	UAVDesc.Texture2DArray.ArraySize		= 6;
	UAVDesc.Texture2DArray.FirstArraySlice	= 0;
	StagingTexture->SetUnorderedAccessView(TSharedPtr(RenderingAPI::Get().CreateUnorderedAccessView(nullptr, StagingTexture->GetResource(), &UAVDesc)), 0);

	TextureProps.Flags = D3D12_RESOURCE_FLAG_NONE;

	TUniquePtr<D3D12Texture> Texture = TUniquePtr(RenderingAPI::Get().CreateTexture(TextureProps));
	if (!Texture)
	{
		return nullptr;
	}

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
	SRVDesc.Format							= Format;
	SRVDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
	SRVDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.TextureCube.MipLevels			= MipLevels;
	SRVDesc.TextureCube.MostDetailedMip		= 0;
	SRVDesc.TextureCube.ResourceMinLODClamp	= 0.0f;
	Texture->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(Texture->GetResource(), &SRVDesc)), 0);

	// Generate PipelineState at first run
	if (!GlobalFactoryData.PanoramaPSO)
	{
		// Create Mip-Gen PSO
		Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/CubeMapGen.hlsl", "Main", "cs_6_0");
		if (!CSBlob)
		{
			return false;
		}

		ComputePipelineStateProperties GenCubeProperties = { };
		GenCubeProperties.DebugName		= "Generate CubeMap Pipeline";
		GenCubeProperties.RootSignature = nullptr;
		GenCubeProperties.CSBlob		= CSBlob.Get();

		GlobalFactoryData.PanoramaPSO = RenderingAPI::Get().CreateComputePipelineState(GenCubeProperties);
		if (!GlobalFactoryData.PanoramaPSO)
		{
			return false;
		}

		GlobalFactoryData.PanoramaRootSignature = RenderingAPI::Get().CreateRootSignature(CSBlob.Get());
		if (!GlobalFactoryData.PanoramaRootSignature)
		{
			return false;
		}

		GlobalFactoryData.PanoramaRootSignature->SetDebugName("Generate CubeMap RootSignature");
	}

	// Create needed interfaces
	TUniquePtr<D3D12DescriptorTable> SrvDescriptorTable = TUniquePtr(RenderingAPI::Get().CreateDescriptorTable(1));
	SrvDescriptorTable->SetShaderResourceView(PanoramaSource->GetShaderResourceView(0).Get(), 0);
	SrvDescriptorTable->CopyDescriptors();

	TUniquePtr<D3D12DescriptorTable> UavDescriptorTable = TUniquePtr(RenderingAPI::Get().CreateDescriptorTable(1));
	UavDescriptorTable->SetUnorderedAccessView(StagingTexture->GetUnorderedAccessView(0).Get(), 0);
	UavDescriptorTable->CopyDescriptors();

	TSharedPtr<D3D12ImmediateCommandList> CommandList = RenderingAPI::StaticGetImmediateCommandList();
	CommandList->TransitionBarrier(PanoramaSource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	CommandList->SetPipelineState(GlobalFactoryData.PanoramaPSO->GetPipeline());
	CommandList->SetComputeRootSignature(GlobalFactoryData.PanoramaRootSignature->GetRootSignature());
	
	struct ConstantBuffer
	{
		uint32 CubeMapSize;
	} CB0;
	CB0.CubeMapSize = CubeMapSize;

	CommandList->SetComputeRoot32BitConstants(&CB0, 1, 0, 0);
	CommandList->BindGlobalOnlineDescriptorHeaps();
	CommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 1);
	CommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableStartHandle(), 2);

	uint32 ThreadsX = Math::DivideByMultiple(CubeMapSize, 16);
	uint32 ThreadsY = Math::DivideByMultiple(CubeMapSize, 16);
	CommandList->Dispatch(ThreadsX, ThreadsY, 6);

	CommandList->TransitionBarrier(PanoramaSource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
	CommandList->TransitionBarrier(Texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	CommandList->CopyResource(Texture.Get(), StagingTexture.Get());

	if (GenerateMipLevels)
	{
		CommandList->GenerateMips(Texture.Get());
	}

	CommandList->TransitionBarrier(Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CommandList->Flush();
	CommandList->WaitForCompletion();

	return Texture.Release();
}
