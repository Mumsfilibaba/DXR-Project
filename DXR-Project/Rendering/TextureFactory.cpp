#include "TextureFactory.h"

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
	std::unique_ptr<D3D12ComputePipelineState> GenerateMipsPSO;
	std::unique_ptr<D3D12RootSignature> GenerateMipsRootSignature;

	std::unique_ptr<D3D12ComputePipelineState> PanoramaPSO;
	std::unique_ptr<D3D12RootSignature> PanoramaRootSignature;
};

static TextureFactoryData GlobalFactoryData;

template <typename T>
inline T DivideByMultiple(T Value, Uint32 Alignment)
{
	return static_cast<T>((Value + Alignment - 1) / Alignment);
}

D3D12Texture* TextureFactory::LoadFromFile(D3D12Device* Device, const std::string& Filepath, Uint32 CreateFlags, DXGI_FORMAT Format)
{
	Int32 Width			= 0;
	Int32 Height		= 0;
	Int32 ChannelCount	= 0;

	// Load based on format
	std::unique_ptr<Byte> Pixels;
	if (Format == DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		Pixels = std::unique_ptr<Byte>(stbi_load(Filepath.c_str(), &Width, &Height, &ChannelCount, 4));
	}
	else if (Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
	{
		Pixels = std::unique_ptr<Byte>(reinterpret_cast<Byte*>(stbi_loadf(Filepath.c_str(), &Width, &Height, &ChannelCount, 4)));
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

	return LoadFromMemory(Device, Pixels.get(), Width, Height, CreateFlags, Format);
}

D3D12Texture* TextureFactory::LoadFromMemory(D3D12Device* Device, const Byte* Pixels, Uint32 Width, Uint32 Height, Uint32 CreateFlags, DXGI_FORMAT Format)
{
	if (Format != DXGI_FORMAT_R8G8B8A8_UNORM && Format != DXGI_FORMAT_R32G32B32A32_FLOAT)
	{
		LOG_ERROR("[TextureFactory]: Format not supported");
		return nullptr;
	}

	const bool GenerateMipLevels	= CreateFlags & ETextureFactoryFlags::TEXTURE_FACTORY_FLAGS_GENERATE_MIPS;
	const Uint32 MipLevels			= GenerateMipLevels ? std::min<Uint32>(std::log2<Uint32>(Width), std::log2<Uint32>(Height)) : 1;

	VALIDATE(MipLevels != 0);

	// Create texture
	TextureProperties TextureProps = { };
	TextureProps.Flags			= GenerateMipLevels ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	TextureProps.Width			= static_cast<Uint16>(Width);
	TextureProps.Height			= static_cast<Uint16>(Height);
	TextureProps.ArrayCount		= 1;
	TextureProps.MipLevels		= static_cast<Uint16>(MipLevels);
	TextureProps.Format			= Format;
	TextureProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	TextureProps.MemoryType		= EMemoryType::MEMORY_TYPE_DEFAULT;

	std::unique_ptr<D3D12Texture> Texture = std::unique_ptr<D3D12Texture>(new D3D12Texture(Device));
	if (!Texture->Initialize(TextureProps))
	{
		return nullptr;
	}

	// Create UploadBuffer
	const Uint32 UploadStride	= (Format == DXGI_FORMAT_R8G8B8A8_UNORM) ? 4 : 16;
	const Uint32 UploadPitch	= (Width * UploadStride + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
	const Uint32 UploadSize		= Height * UploadPitch;

	BufferProperties UploadBufferProps = { };
	UploadBufferProps.Name			= "UploadBuffer";
	UploadBufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	UploadBufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
	UploadBufferProps.SizeInBytes	= UploadSize;
	UploadBufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

	std::unique_ptr<D3D12Buffer> UploadBuffer = std::unique_ptr<D3D12Buffer>(new D3D12Buffer(Device));
	if (UploadBuffer->Initialize(UploadBufferProps))
	{
		void* Memory = UploadBuffer->Map();
		for (Uint32 Y = 0; Y < Height; Y++)
		{
			memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Memory) + Y * UploadPitch), Pixels + (Y * Width * UploadStride), Width * UploadStride);
		}
		UploadBuffer->Unmap();
	}
	else
	{
		return nullptr;
	}

	// ShaderResourceView
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format						= Format;
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Texture2D.MipLevels			= MipLevels;
	SrvDesc.Texture2D.MostDetailedMip	= 0;
	Texture->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device, Texture->GetResource(), &SrvDesc), 0);

	std::unique_ptr<D3D12Texture> TempTexture;
	if (GenerateMipLevels)
	{
		TextureProps.DebugName	= "Staging Texture";
		TextureProps.Flags		= D3D12_RESOURCE_FLAG_NONE;

		TempTexture = std::unique_ptr<D3D12Texture>(new D3D12Texture(Device));
		if (!TempTexture->Initialize(TextureProps))
		{
			return nullptr;
		}
		else
		{
			TempTexture->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device, TempTexture->GetResource(), &SrvDesc), 0);
		}
	}

	// Create Resources for generating Miplevels
	const Uint32 MipLevelsPerDispatch = 4;
	Uint32 NumDispatches = MipLevels / MipLevelsPerDispatch;

	std::unique_ptr<D3D12UnorderedAccessView> NULLView;
	std::unique_ptr<D3D12DescriptorTable> SrvDescriptorTable;
	std::vector<std::unique_ptr<D3D12DescriptorTable>> UavDescriptorTables;
	if (GenerateMipLevels)
	{
		Uint32 MiplevelsLastDispatch = MipLevels - (MipLevelsPerDispatch * NumDispatches);
		if (MiplevelsLastDispatch > 0)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = { };
			UavDesc.Format					= Format;
			UavDesc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
			UavDesc.Texture2D.MipSlice		= 0;
			UavDesc.Texture2D.PlaneSlice	= 0;

			NULLView = std::make_unique<D3D12UnorderedAccessView>(Device, nullptr, nullptr, &UavDesc);

			NumDispatches++;
			UavDescriptorTables.resize(NumDispatches);
		}

		if (!GlobalFactoryData.GenerateMipsPSO)
		{
			// Create Mip-Gen PSO
			Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::Get()->CompileFromFile("Shaders/MipMapGen.hlsl", "main", "cs_6_0");
			if (!CSBlob)
			{
				return false;
			}

			ComputePipelineStateProperties GenMipsProperties = { };
			GenMipsProperties.DebugName		= "Generate MipLevels Pipeline";
			GenMipsProperties.RootSignature = nullptr;
			GenMipsProperties.CSBlob		= CSBlob.Get();

			GlobalFactoryData.GenerateMipsPSO = std::unique_ptr<D3D12ComputePipelineState>(new D3D12ComputePipelineState(Device));
			if (!GlobalFactoryData.GenerateMipsPSO->Initialize(GenMipsProperties))
			{
				return false;
			}

			GlobalFactoryData.GenerateMipsRootSignature = std::unique_ptr<D3D12RootSignature>(new D3D12RootSignature(Device));
			if (!GlobalFactoryData.GenerateMipsRootSignature->Initialize(CSBlob.Get()))
			{
				return false;
			}

			GlobalFactoryData.GenerateMipsRootSignature->SetName("Generate MipLevels RootSignature");
		}

		// ShaderResourceView
		SrvDescriptorTable = std::unique_ptr<D3D12DescriptorTable>(new D3D12DescriptorTable(Device, 1));
		SrvDescriptorTable->SetShaderResourceView(Texture->GetShaderResourceView(0).get(), 0);
		SrvDescriptorTable->CopyDescriptors();

		// UnorderedAccessViews
		for (Uint32 MipLevel = 0; MipLevel < MipLevels; MipLevel++)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = { };
			UavDesc.Format					= Format;
			UavDesc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
			UavDesc.Texture2D.MipSlice		= MipLevel;
			UavDesc.Texture2D.PlaneSlice	= 0;

			Texture->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device, nullptr, Texture->GetResource(), &UavDesc), MipLevel);
		}

		Uint32 UAVIndex = 0;
		for (Uint32 I = 0; I < NumDispatches; I++)
		{
			UavDescriptorTables[I] = std::unique_ptr<D3D12DescriptorTable>(new D3D12DescriptorTable(Device, 4));
			for (Uint32 J = 0; J < 4; J++)
			{
				if (UAVIndex < MipLevels)
				{
					UavDescriptorTables[I]->SetUnorderedAccessView(Texture->GetUnorderedAccessView(UAVIndex).get(), J);
					UAVIndex++;
				}
				else
				{
					UavDescriptorTables[I]->SetUnorderedAccessView(NULLView.get(), J);
				}
			}

			UavDescriptorTables[I]->CopyDescriptors();
		}
	}

	// Upload data
	std::unique_ptr<D3D12CommandAllocator> Allocator = std::unique_ptr<D3D12CommandAllocator>(new D3D12CommandAllocator(Device));
	if (!Allocator->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return nullptr;
	}

	std::unique_ptr<D3D12CommandList> CommandList = std::unique_ptr<D3D12CommandList>(new D3D12CommandList(Device));
	if (!CommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator.get(), nullptr))
	{
		return nullptr;
	}

	std::unique_ptr<D3D12CommandQueue> Queue = std::unique_ptr<D3D12CommandQueue>(new D3D12CommandQueue(Device));
	if (!Queue->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return nullptr;
	}

	Allocator->Reset();
	CommandList->Reset(Allocator.get());

	CommandList->TransitionBarrier(Texture.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
	SourceLocation.pResource							= UploadBuffer->GetResource();
	SourceLocation.Type									= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	SourceLocation.PlacedFootprint.Footprint.Format		= Format;
	SourceLocation.PlacedFootprint.Footprint.Width		= Width;
	SourceLocation.PlacedFootprint.Footprint.Height		= Height;
	SourceLocation.PlacedFootprint.Footprint.Depth		= 1;
	SourceLocation.PlacedFootprint.Footprint.RowPitch	= UploadPitch;

	D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
	DestLocation.pResource			= Texture->GetResource();
	DestLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	DestLocation.SubresourceIndex	= 0;

	CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

	if (GenerateMipLevels)
	{
		CommandList->TransitionBarrier(Texture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CommandList->SetPipelineState(GlobalFactoryData.GenerateMipsPSO->GetPipeline());
		CommandList->SetComputeRootSignature(GlobalFactoryData.GenerateMipsRootSignature->GetRootSignature());

		ID3D12DescriptorHeap* GlobalHeap = Device->GetGlobalOnlineResourceHeap()->GetHeap();
		CommandList->SetDescriptorHeaps(&GlobalHeap, 1);
		CommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 1);

		struct ConstantBuffer
		{
			Uint32		SrcMipLevel;
			Uint32		NumMipLevels;
			XMFLOAT2	TexelSize;
		} CB0;

		Uint32 DstWidth		= Width;
		Uint32 DstHeight	= Height;
		CB0.SrcMipLevel		= 0;

		Uint32 RemainingMiplevels = MipLevels;
		for (Uint32 I = 0; I < NumDispatches; I++)
		{
			CB0.TexelSize		= XMFLOAT2(1.0f / static_cast<Float32>(DstWidth), 1.0f / static_cast<Float32>(DstHeight));
			CB0.NumMipLevels	= std::min<Uint32>(4, RemainingMiplevels);

			CommandList->SetComputeRoot32BitConstants(&CB0, 4, 0, 0);
			CommandList->SetComputeRootDescriptorTable(UavDescriptorTables[I]->GetGPUTableStartHandle(), 2);

			Uint32 ThreadsX = DivideByMultiple(DstWidth, 8);
			Uint32 ThreadsY = DivideByMultiple(DstHeight, 8);
			CommandList->Dispatch(ThreadsX, ThreadsY, 1);
			
			CommandList->UnorderedAccessBarrier(Texture.get());

			DstWidth	= DstWidth / 16;
			DstHeight	= DstHeight / 16;

			CB0.SrcMipLevel += 3;
			RemainingMiplevels -= MipLevelsPerDispatch;
		}

		CommandList->TransitionBarrier(Texture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CommandList->TransitionBarrier(TempTexture.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CommandList->CopyResource(TempTexture.get(), Texture.get());
		
		CommandList->TransitionBarrier(TempTexture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		CommandList->TransitionBarrier(Texture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	CommandList->Close();

	Queue->ExecuteCommandList(CommandList.get());
	Queue->WaitForCompletion();

	return Texture.release();
}

D3D12Texture* TextureFactory::CreateTextureCubeFromPanorma(D3D12Device* Device, D3D12Texture* PanoramaSource, Uint32 CubeMapSize, DXGI_FORMAT Format)
{
	VALIDATE(PanoramaSource->GetShaderResourceView(0));

	// Create texture
	TextureProperties TextureProps = { };
	TextureProps.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	TextureProps.Width			= static_cast<Uint16>(CubeMapSize);
	TextureProps.Height			= static_cast<Uint16>(CubeMapSize);
	TextureProps.ArrayCount		= 6;
	TextureProps.MipLevels		= 1;
	TextureProps.Format			= Format;
	TextureProps.MemoryType		= EMemoryType::MEMORY_TYPE_DEFAULT;
	TextureProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;

	std::unique_ptr<D3D12Texture> StagingTexture = std::unique_ptr<D3D12Texture>(new D3D12Texture(Device));
	if (!StagingTexture->Initialize(TextureProps))
	{
		return nullptr;
	}

	TextureProps.Flags = D3D12_RESOURCE_FLAG_NONE;

	std::unique_ptr<D3D12Texture> Texture = std::unique_ptr<D3D12Texture>(new D3D12Texture(Device));
	if (!Texture->Initialize(TextureProps))
	{
		return nullptr;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = { };
	UavDesc.Format							= Format;
	UavDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	UavDesc.Texture2DArray.ArraySize		= 6;
	UavDesc.Texture2DArray.FirstArraySlice	= 0;
	StagingTexture->SetUnorderedAccessView(std::make_unique<D3D12UnorderedAccessView>(Device, nullptr, StagingTexture->GetResource(), &UavDesc), 0);

	// Generate PipelineState at first run
	if (!GlobalFactoryData.PanoramaPSO)
	{
		// Create Mip-Gen PSO
		Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::Get()->CompileFromFile("Shaders/CubeMapGen.hlsl", "main", "cs_6_0");
		if (!CSBlob)
		{
			return false;
		}

		ComputePipelineStateProperties GenCubeProperties = { };
		GenCubeProperties.DebugName		= "Generate CubeMap Pipeline";
		GenCubeProperties.RootSignature = nullptr;
		GenCubeProperties.CSBlob		= CSBlob.Get();

		GlobalFactoryData.PanoramaPSO = std::unique_ptr<D3D12ComputePipelineState>(new D3D12ComputePipelineState(Device));
		if (!GlobalFactoryData.PanoramaPSO->Initialize(GenCubeProperties))
		{
			return false;
		}

		GlobalFactoryData.PanoramaRootSignature = std::unique_ptr<D3D12RootSignature>(new D3D12RootSignature(Device));
		if (!GlobalFactoryData.PanoramaRootSignature->Initialize(CSBlob.Get()))
		{
			return false;
		}

		GlobalFactoryData.PanoramaRootSignature->SetName("Generate CubeMap RootSignature");
	}

	// Create needed interfaces
	std::unique_ptr<D3D12DescriptorTable> SrvDescriptorTable = std::make_unique<D3D12DescriptorTable>(Device, 1);
	SrvDescriptorTable->SetShaderResourceView(PanoramaSource->GetShaderResourceView(0).get(), 0);
	SrvDescriptorTable->CopyDescriptors();

	std::unique_ptr<D3D12DescriptorTable> UavDescriptorTable = std::make_unique<D3D12DescriptorTable>(Device, 1);
	UavDescriptorTable->SetUnorderedAccessView(StagingTexture->GetUnorderedAccessView(0).get(), 0);
	UavDescriptorTable->CopyDescriptors();

	std::unique_ptr<D3D12CommandAllocator> Allocator = std::unique_ptr<D3D12CommandAllocator>(new D3D12CommandAllocator(Device));
	if (!Allocator->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return nullptr;
	}

	std::unique_ptr<D3D12CommandList> CommandList = std::unique_ptr<D3D12CommandList>(new D3D12CommandList(Device));
	if (!CommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator.get(), nullptr))
	{
		return nullptr;
	}

	std::unique_ptr<D3D12CommandQueue> Queue = std::unique_ptr<D3D12CommandQueue>(new D3D12CommandQueue(Device));
	if (!Queue->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return nullptr;
	}

	// Generate Cube
	Allocator->Reset();
	CommandList->Reset(Allocator.get());

	CommandList->TransitionBarrier(PanoramaSource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(StagingTexture.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	CommandList->SetPipelineState(GlobalFactoryData.PanoramaPSO->GetPipeline());
	CommandList->SetComputeRootSignature(GlobalFactoryData.PanoramaRootSignature->GetRootSignature());
	
	struct ConstantBuffer
	{
		Uint32 CubeMapSize;
	} CB0;
	CB0.CubeMapSize = CubeMapSize;

	CommandList->SetComputeRoot32BitConstants(&CB0, 1, 0, 0);
	
	ID3D12DescriptorHeap* GlobalHeap = Device->GetGlobalOnlineResourceHeap()->GetHeap();
	CommandList->SetDescriptorHeaps(&GlobalHeap, 1);
	CommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 1);
	CommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableStartHandle(), 2);

	Uint32 ThreadsX = DivideByMultiple(CubeMapSize, 16);
	Uint32 ThreadsY = DivideByMultiple(CubeMapSize, 16);
	CommandList->Dispatch(ThreadsX, ThreadsY, 6);

	CommandList->TransitionBarrier(PanoramaSource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(StagingTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
	CommandList->TransitionBarrier(Texture.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	CommandList->CopyResource(Texture.get(), StagingTexture.get());

	CommandList->TransitionBarrier(Texture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CommandList->Close();

	Queue->ExecuteCommandList(CommandList.get());
	Queue->WaitForCompletion();

	// ShaderResourceView
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format							= Format;
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.TextureCube.MipLevels			= 1;
	SrvDesc.TextureCube.MostDetailedMip		= 0;
	SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	Texture->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device, Texture->GetResource(), &SrvDesc), 0);

	return Texture.release();
}
