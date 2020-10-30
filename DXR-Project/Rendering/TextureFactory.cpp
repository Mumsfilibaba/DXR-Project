#include "TextureFactory.h"
#include "Types.h"

#include "RenderingCore/CommandList.h"
#include "RenderingCore/PipelineState.h"
#include "RenderingCore/RenderingAPI.h"


#ifdef min
	#undef min
#endif

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/*
* TextureFactoryData
*/

struct TextureFactoryData
{
	CommandList	CmdList;
	TSharedRef<ComputePipelineState> PanoramaPSO;
} static GlobalFactoryData;

/*
* TextureFactory
*/

bool TextureFactory::Initialize()
{
	return true;
}

void TextureFactory::Release()
{
}

Texture2D* TextureFactory::LoadFromFile(const std::string& Filepath, Uint32 CreateFlags, EFormat Format)
{
	Int32 Width			= 0;
	Int32 Height		= 0;
	Int32 ChannelCount	= 0;

	// Load based on format
	TUniquePtr<Byte> Pixels;
	if (Format == EFormat::Format_R8G8B8A8_Unorm)
	{
		Pixels = TUniquePtr<Byte>(stbi_load(Filepath.c_str(), &Width, &Height, &ChannelCount, 4));
	}
	else if (Format == EFormat::Format_R32G32B32A32_Float)
	{
		Pixels = TUniquePtr<Byte>(reinterpret_cast<Byte*>(stbi_loadf(Filepath.c_str(), &Width, &Height, &ChannelCount, 4)));
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

Texture2D* TextureFactory::LoadFromMemory(const Byte* Pixels, Uint32 Width, Uint32 Height, Uint32 CreateFlags, EFormat Format)
{
	if (Format != EFormat::Format_R8G8B8A8_Unorm && Format != EFormat::Format_R32G32B32A32_Float)
	{
		LOG_ERROR("[TextureFactory]: Format not supported");
		return nullptr;
	}

	const bool GenerateMipLevels = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
	const Uint32 MipLevels = GenerateMipLevels ? std::min<Uint32>(std::log2<Uint32>(Width), std::log2<Uint32>(Height)) : 1;

	VALIDATE(MipLevels != 0);

	const Uint32 Stride		= (Format == EFormat::Format_R8G8B8A8_Unorm) ? 4 : 16;
	const Uint32 RowPitch	= Width * Stride;
	
	VALIDATE(RowPitch > 0);
	
	ResourceData InitalData = ResourceData((const VoidPtr)Pixels, Width * Stride);
	TSharedRef<Texture2D> Texture = RenderingAPI::CreateTexture2D(
		&InitalData,
		Format, 
		TextureUsage_Default | TextureUsage_SRV,
		Width, 
		Height, 
		MipLevels, 
		1);

	if (!Texture)
	{
		return nullptr;
	}

	if (GenerateMipLevels)
	{
		CommandList& CmdList = GlobalFactoryData.CmdList;
		CmdList.TransitionTexture(Texture.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_CopyDest);	
		CmdList.GenerateMips(Texture.Get());
		CmdList.TransitionTexture(Texture.Get(), EResourceState::ResourceState_CopyDest, EResourceState::ResourceState_PixelShaderResource);
	
		CommandListExecutor& Executor = RenderingAPI::GetCommandListExecutor();
		Executor.ExecuteCommandList(CmdList);
	}

	return Texture.ReleaseOwnerShip();
}

TextureCube* TextureFactory::CreateTextureCubeFromPanorma(Texture2D* PanoramaSource, Uint32 CubeMapSize, Uint32 CreateFlags, EFormat Format)
{
	const bool GenerateMipLevels = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
	const Uint16 MipLevels = (GenerateMipLevels) ? static_cast<Uint16>(std::log2(CubeMapSize)) : 1U;

	// Create statging texture
	TSharedRef<TextureCube> StagingTexture = RenderingAPI::CreateTextureCube(
		nullptr, 
		Format, 
		TextureUsage_Default | TextureUsage_UAV ,
		CubeMapSize, 
		MipLevels, 
		1);
	if (!StagingTexture)
	{
		return nullptr;
	}

	// Create UAV
	TSharedRef<UnorderedAccessView> Uav = RenderingAPI::CreateUnorderedAccessView(StagingTexture.Get(), Format, 0);
	if (!Uav)
	{
		return nullptr;
	}

	// Create texture
	TSharedRef<TextureCube> Texture = RenderingAPI::CreateTextureCube(
		nullptr, 
		Format, 
		TextureUsage_SRV | TextureUsage_Default, 
		CubeMapSize, 
		MipLevels, 
		1);
	if (!Texture)
	{
		return nullptr;
	}

	// Generate PipelineState at first run
	//if (!GlobalFactoryData.PanoramaPSO)
	//{
	//	// Create Mip-Gen PSO
	//	Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/CubeMapGen.hlsl", "Main", "cs_6_0");
	//	if (!CSBlob)
	//	{
	//		return false;
	//	}

	//	ComputePipelineStateProperties GenCubeProperties = { };
	//	GenCubeProperties.DebugName		= "Generate CubeMap Pipeline";
	//	GenCubeProperties.RootSignature = nullptr;
	//	GenCubeProperties.CSBlob		= CSBlob.Get();

	//	GlobalFactoryData.PanoramaPSO = RenderingAPI::Get().CreateComputePipelineState(GenCubeProperties);
	//	if (!GlobalFactoryData.PanoramaPSO)
	//	{
	//		return false;
	//	}

	//	GlobalFactoryData.PanoramaRootSignature = RenderingAPI::Get().CreateRootSignature(CSBlob.Get());
	//	if (!GlobalFactoryData.PanoramaRootSignature)
	//	{
	//		return false;
	//	}

	//	GlobalFactoryData.PanoramaRootSignature->SetDebugName("Generate CubeMap RootSignature");
	//}

	//// Create needed interfaces
	//TSharedPtr<D3D12ImmediateCommandList> CommandList = RenderingAPI::StaticGetImmediateCommandList();
	//CommandList->TransitionBarrier(PanoramaSource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//CommandList->TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//CommandList->SetPipelineState(GlobalFactoryData.PanoramaPSO->GetPipeline());
	//CommandList->SetComputeRootSignature(GlobalFactoryData.PanoramaRootSignature->GetRootSignature());
	//
	//struct ConstantBuffer
	//{
	//	Uint32 CubeMapSize;
	//} CB0;
	//CB0.CubeMapSize = CubeMapSize;

	//CommandList->SetComputeRoot32BitConstants(&CB0, 1, 0, 0);
	//CommandList->BindGlobalOnlineDescriptorHeaps();
	//CommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 1);
	//CommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableStartHandle(), 2);

	//Uint32 ThreadsX = DivideByMultiple(CubeMapSize, 16);
	//Uint32 ThreadsY = DivideByMultiple(CubeMapSize, 16);
	//CommandList->Dispatch(ThreadsX, ThreadsY, 6);

	//CommandList->TransitionBarrier(PanoramaSource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//CommandList->TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
	//CommandList->TransitionBarrier(Texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	//CommandList->CopyResource(Texture.Get(), StagingTexture.Get());

	//if (GenerateMipLevels)
	//{
	//	CommandList->GenerateMips(Texture.Get());
	//}

	//CommandList->TransitionBarrier(Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	//CommandList->Flush();
	//CommandList->WaitForCompletion();

	//return Texture.Release();

	return nullptr;
}
