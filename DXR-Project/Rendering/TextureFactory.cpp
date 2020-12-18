#include "TextureFactory.h"
#include "Types.h"

#include "RenderingCore/CommandList.h"
#include "RenderingCore/PipelineState.h"
#include "RenderingCore/RenderingAPI.h"
#include "RenderingCore/Shader.h"

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
	// Compile and create shader
	TArray<Uint8> Code;
	if (!ShaderCompiler::CompileFromFile(
		"Shaders/CubeMapGen.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Compute,
		EShaderModel::ShaderModel_6_0,
		Code))
	{
		return false;
	}

	ComputeShader* Shader = RenderingAPI::CreateComputeShader(Code);
	if (!Shader)
	{
		return false;
	}

	// Create pipeline
	GlobalFactoryData.PanoramaPSO = RenderingAPI::CreateComputePipelineState(ComputePipelineStateCreateInfo(Shader));
	if (GlobalFactoryData.PanoramaPSO)
	{
		GlobalFactoryData.PanoramaPSO->SetName("Generate CubeMap RootSignature");
		return true;
	}
	else
	{
		return false;
	}
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
	
		CommandListExecutor::ExecuteCommandList(CmdList);
	}

	return Texture.ReleaseOwnerShip();
}

TextureCube* TextureFactory::CreateTextureCubeFromPanorma(Texture2D* PanoramaSource, Uint32 CubeMapSize, Uint32 CreateFlags, EFormat Format)
{
	const bool		GenerateMipLevels = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
	const Uint16	MipLevels = (GenerateMipLevels) ? static_cast<Uint16>(std::log2(CubeMapSize)) : 1U;

	// Create statging texture
	TSharedRef<TextureCube> StagingTexture = RenderingAPI::CreateTextureCube(
		nullptr, 
		Format, 
		TextureUsage_Default | TextureUsage_UAV,
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

	// Create needed interfaces
	CommandList& CmdList = GlobalFactoryData.CmdList;
	CmdList.Begin();
	
	CmdList.TransitionTexture(PanoramaSource, EResourceState::ResourceState_PixelShaderResource, EResourceState::ResourceState_NonPixelShaderResource);
	CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_NonPixelShaderResource);

	CmdList.BindComputePipelineState(GlobalFactoryData.PanoramaPSO.Get());

	struct ConstantBuffer
	{
		Uint32 CubeMapSize;
	} CB0;
	CB0.CubeMapSize = CubeMapSize;


	// TODO: How to work with constants and resources
	//CommandList->SetComputeRoot32BitConstants(&CB0, 1, 0, 0);
	//CommandList->BindGlobalOnlineDescriptorHeaps();
	//CommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 1);
	//CommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableStartHandle(), 2);

	Uint32 ThreadsX = DivideByMultiple(CubeMapSize, 16);
	Uint32 ThreadsY = DivideByMultiple(CubeMapSize, 16);
	CmdList.Dispatch(ThreadsX, ThreadsY, 6);

	CmdList.TransitionTexture(PanoramaSource, EResourceState::ResourceState_NonPixelShaderResource, EResourceState::ResourceState_PixelShaderResource);
	CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::ResourceState_NonPixelShaderResource, EResourceState::ResourceState_PixelShaderResource);
	CmdList.TransitionTexture(Texture.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_CopyDest);

	CmdList.CopyTexture(Texture.Get(), StagingTexture.Get(), CopyTextureInfo());

	if (GenerateMipLevels)
	{
		CmdList.GenerateMips(Texture.Get());
	}

	CmdList.TransitionTexture(Texture.Get(), EResourceState::ResourceState_CopyDest, EResourceState::ResourceState_PixelShaderResource);
	CommandListExecutor::ExecuteCommandList(CmdList);

	return Texture.ReleaseOwnerShip();
}
