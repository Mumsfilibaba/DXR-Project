#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12ComputePipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"

#include <algorithm>

D3D12CommandList::D3D12CommandList(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, CommandList(nullptr)
	, DXRCommandList(nullptr)
	, DeferredResourceBarriers()
{
}

D3D12CommandList::~D3D12CommandList()
{
	SAFEDELETE(UploadBuffer);
}

bool D3D12CommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline)
{
	HRESULT hResult = Device->GetDevice()->CreateCommandList(0, Type, Allocator->GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CommandList));
	if (SUCCEEDED(hResult))
	{
		CommandList->Close();
		LOG_INFO("[D3D12CommandList]: Created CommandList");

		if (FAILED(CommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
		{
			LOG_ERROR("[D3D12CommandList]: FAILED to retrive DXR-CommandList");
			return false;
		}

		return CreateUploadBuffer();
	}
	else
	{
		LOG_ERROR("[D3D12CommandList]: FAILED to create CommandList");
		return false;
	}
}

void D3D12CommandList::GenerateMips(D3D12Texture* Dest)
{
	using namespace Microsoft::WRL;

	const D3D12_RESOURCE_DESC& Desc = Dest->GetDesc();
	VALIDATE(Desc.MipLevels > 1);

	// Check Type
	bool IsTextureCube = false;
	if (Dest->GetShaderResourceView(0)->GetDesc().ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
	{
		IsTextureCube = true;
	}

	// Create staging texture
	TextureProperties StagingTextureProps;
	StagingTextureProps.DebugName			= "StagingTexture";
	StagingTextureProps.Flags				= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	StagingTextureProps.Format				= Desc.Format;
	StagingTextureProps.Width				= static_cast<Uint16>(Desc.Width);
	StagingTextureProps.Height				= static_cast<Uint16>(Desc.Height);
	StagingTextureProps.ArrayCount			= static_cast<Uint16>(Desc.DepthOrArraySize);
	StagingTextureProps.MemoryType			= Dest->GetMemoryType();
	StagingTextureProps.InitalState			= D3D12_RESOURCE_STATE_COMMON;
	StagingTextureProps.MipLevels			= Desc.MipLevels;
	StagingTextureProps.OptimizedClearValue	= nullptr;
	StagingTextureProps.SampleCount			= 1;

	TUniquePtr<D3D12Texture> StagingTexture = MakeUnique<D3D12Texture>(Device);
	if (!StagingTexture->Initialize(StagingTextureProps))
	{
		LOG_ERROR("[D3D12CommandList] Failed to create StagingTexture for GenerateMips");
		return;
	}

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
	SRVDesc.Format						= Desc.Format;
	SRVDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (IsTextureCube)
	{
		SRVDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
		SRVDesc.TextureCube.MipLevels			= Desc.MipLevels;
		SRVDesc.TextureCube.MostDetailedMip		= 0;
		SRVDesc.TextureCube.ResourceMinLODClamp	= 0.0f;
	}
	else
	{
		SRVDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels			= Desc.MipLevels;
		SRVDesc.Texture2D.MostDetailedMip	= 0;
	}

	StagingTexture->SetShaderResourceView(MakeShared<D3D12ShaderResourceView>(Device, StagingTexture->GetResource(), &SRVDesc), 0);

	// Create UAVs
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = { };
	UAVDesc.Format = Desc.Format;
	if (IsTextureCube)
	{
		UAVDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVDesc.Texture2DArray.ArraySize		= 6;
		UAVDesc.Texture2DArray.FirstArraySlice	= 0;
		UAVDesc.Texture2DArray.PlaneSlice		= 0;
	}
	else
	{
		UAVDesc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.PlaneSlice	= 0;
	}

	for (Uint32 i = 0; i < Desc.MipLevels; i++)
	{
		if (IsTextureCube)
		{
			UAVDesc.Texture2DArray.MipSlice = i;
		}
		else
		{
			UAVDesc.Texture2D.MipSlice = i;
		}

		StagingTexture->SetUnorderedAccessView(MakeShared<D3D12UnorderedAccessView>(Device, nullptr, StagingTexture->GetResource(), &UAVDesc), i);
	}

	// Create PSO and RS
	ComPtr<ID3D12PipelineState> PipelineState;
	if (IsTextureCube)
	{
		if (!MipGenHelper.GenerateMipsTexCube_PSO)
		{
			Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", "Main", "cs_6_0");
			if (!CSBlob)
			{
				return;
			}

			ComputePipelineStateProperties GenMipsProperties = { };
			GenMipsProperties.DebugName		= "Generate MipLevels Pipeline TexCube";
			GenMipsProperties.RootSignature	= nullptr;
			GenMipsProperties.CSBlob		= CSBlob.Get();

			MipGenHelper.GenerateMipsTexCube_PSO = MakeUnique<D3D12ComputePipelineState>(Device);
			if (!MipGenHelper.GenerateMipsTexCube_PSO->Initialize(GenMipsProperties))
			{
				return;
			}

			// Create rootsignature
			if (!MipGenHelper.GenerateMipsRootSignature)
			{
				MipGenHelper.GenerateMipsRootSignature = MakeUnique<D3D12RootSignature>(Device);
				if (MipGenHelper.GenerateMipsRootSignature->Initialize(CSBlob.Get()))
				{
					MipGenHelper.GenerateMipsRootSignature->SetDebugName("Generate MipLevels RootSignature");
				}
				else
				{
					return;
				}
			}
		}

		PipelineState = MipGenHelper.GenerateMipsTexCube_PSO->GetPipeline();
	}
	else
	{
		if (!MipGenHelper.GenerateMipsTex2D_PSO)
		{
			Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", "Main", "cs_6_0");
			if (!CSBlob)
			{
				return;
			}

			ComputePipelineStateProperties GenMipsProperties = { };
			GenMipsProperties.DebugName		= "Generate MipLevels Pipeline Tex2D";
			GenMipsProperties.RootSignature = nullptr;
			GenMipsProperties.CSBlob		= CSBlob.Get();

			MipGenHelper.GenerateMipsTex2D_PSO = MakeUnique<D3D12ComputePipelineState>(Device);
			if (!MipGenHelper.GenerateMipsTex2D_PSO->Initialize(GenMipsProperties))
			{
				return;
			}

			// Create rootsignature
			if (!MipGenHelper.GenerateMipsRootSignature)
			{
				MipGenHelper.GenerateMipsRootSignature = MakeUnique<D3D12RootSignature>(Device);
				if (MipGenHelper.GenerateMipsRootSignature->Initialize(CSBlob.Get()))
				{
					MipGenHelper.GenerateMipsRootSignature->SetDebugName("Generate MipLevels RootSignature");
				}
				else
				{
					return;
				}
			}
		}

		PipelineState = MipGenHelper.GenerateMipsTex2D_PSO->GetPipeline();
	}

	// Create Resources for generating Miplevels
	const Uint32 MipLevelsPerDispatch = 4;
	Uint32 NumDispatches = Desc.MipLevels / MipLevelsPerDispatch;

	Uint32 MiplevelsLastDispatch = Desc.MipLevels - (MipLevelsPerDispatch * NumDispatches);
	if (MiplevelsLastDispatch > 0)
	{
		if (!MipGenHelper.NULLView)
		{
			UAVDesc.Format					= Desc.Format;
			UAVDesc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice		= 0;
			UAVDesc.Texture2D.PlaneSlice	= 0;
			MipGenHelper.NULLView = MakeUnique<D3D12UnorderedAccessView>(Device, nullptr, nullptr, &UAVDesc);
		}

		NumDispatches++;
	}

	// Resize if necessary
	if (MipGenHelper.UAVDescriptorTables.Size() < NumDispatches)
	{
		MipGenHelper.UAVDescriptorTables.Resize(NumDispatches);
	}

	// Bind ShaderResourceView
	if (!MipGenHelper.SRVDescriptorTable)
	{
		MipGenHelper.SRVDescriptorTable = MakeUnique<D3D12DescriptorTable>(Device, 1);
	}

	MipGenHelper.SRVDescriptorTable->SetShaderResourceView(StagingTexture->GetShaderResourceView(0).Get(), 0);
	MipGenHelper.SRVDescriptorTable->CopyDescriptors();

	// Bind UnorderedAccessViews
	Uint32 UAVIndex = 0;
	for (Uint32 i = 0; i < NumDispatches; i++)
	{
		if (!MipGenHelper.UAVDescriptorTables[i])
		{
			MipGenHelper.UAVDescriptorTables[i] = MakeUnique<D3D12DescriptorTable>(Device, 4);
		}

		for (Uint32 j = 0; j < MipLevelsPerDispatch; j++)
		{
			if (UAVIndex < Desc.MipLevels)
			{
				MipGenHelper.UAVDescriptorTables[i]->SetUnorderedAccessView(StagingTexture->GetUnorderedAccessView(UAVIndex).Get(), j);
				UAVIndex++;
			}
			else
			{
				MipGenHelper.UAVDescriptorTables[i]->SetUnorderedAccessView(MipGenHelper.NULLView.Get(), j);
			}
		}

		MipGenHelper.UAVDescriptorTables[i]->CopyDescriptors();
	}

	// Copy the source over to the staging texture
	TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	CopyResource(StagingTexture.Get(), Dest);

	TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	SetPipelineState(PipelineState.Get());
	SetComputeRootSignature(MipGenHelper.GenerateMipsRootSignature->GetRootSignature());

	ID3D12DescriptorHeap* GlobalHeap = Device->GetGlobalOnlineResourceHeap()->GetHeap();
	SetDescriptorHeaps(&GlobalHeap, 1);
	SetComputeRootDescriptorTable(MipGenHelper.SRVDescriptorTable->GetGPUTableStartHandle(), 1);

	struct ConstantBuffer
	{
		Uint32		SrcMipLevel;
		Uint32		NumMipLevels;
		XMFLOAT2	TexelSize;
	} CB0;

	Uint32 DstWidth		= static_cast<Uint32>(Desc.Width);
	Uint32 DstHeight	= Desc.Height;
	CB0.SrcMipLevel		= 0;

	const Uint32 ThreadsZ = IsTextureCube ? 6 : 1;
	Uint32 RemainingMiplevels = Desc.MipLevels;
	for (Uint32 i = 0; i < NumDispatches; i++)
	{
		CB0.TexelSize		= XMFLOAT2(1.0f / static_cast<Float32>(DstWidth), 1.0f / static_cast<Float32>(DstHeight));
		CB0.NumMipLevels	= std::min<Uint32>(4, RemainingMiplevels);

		SetComputeRoot32BitConstants(&CB0, 4, 0, 0);
		SetComputeRootDescriptorTable(MipGenHelper.UAVDescriptorTables[i]->GetGPUTableStartHandle(), 2);

		const Uint32 ThreadsX = DivideByMultiple(DstWidth, 8);
		const Uint32 ThreadsY = DivideByMultiple(DstHeight, 8);
		Dispatch(ThreadsX, ThreadsY, ThreadsZ);

		UnorderedAccessBarrier(StagingTexture.Get());

		DstWidth	= DstWidth / 16;
		DstHeight	= DstHeight / 16;

		CB0.SrcMipLevel += 3;
		RemainingMiplevels -= MipLevelsPerDispatch;
	}

	TransitionBarrier(Dest, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);

	CopyResource(Dest, StagingTexture.Get());

	DeferDestruction(StagingTexture.Get());
}

void D3D12CommandList::FlushDeferredResourceBarriers()
{
	if (!DeferredResourceBarriers.IsEmpty())
	{
		// TODO: Remove unnecessary barriers

		CommandList->ResourceBarrier(static_cast<UINT>(DeferredResourceBarriers.Size()), DeferredResourceBarriers.Data());
		DeferredResourceBarriers.Clear();
	}
}

void D3D12CommandList::BindGlobalOnlineDescriptorHeaps()
{
	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalOnlineResourceHeap()->GetHeap() };
	SetDescriptorHeaps(DescriptorHeaps, 1);
}

void D3D12CommandList::UploadBufferData(D3D12Buffer* Dest, const Uint32 DestOffset, const void* Src, const Uint32 SizeInBytes)
{
	const Uint32 NewOffset = UploadBufferOffset + SizeInBytes;
	if (NewOffset >= UploadBuffer->GetSizeInBytes())
	{
		// Destroy old buffer
		DeferDestruction(UploadBuffer);
		SAFEDELETE(UploadBuffer);

		// Create new one
		CreateUploadBuffer(NewOffset);
	}

	// Copy to GPU buffer
	memcpy(UploadPointer + UploadBufferOffset, Src, SizeInBytes);
	// Copy to Dest
	CopyBuffer(Dest, DestOffset, UploadBuffer, UploadBufferOffset, SizeInBytes);

	UploadBufferOffset = NewOffset;
}

void D3D12CommandList::UploadTextureData(
	class D3D12Texture* Dest, 
	const void* Src, 
	DXGI_FORMAT Format, 
	const Uint32 Width, 
	const Uint32 Height, 
	const Uint32 Depth, 
	const Uint32 Stride, 
	const Uint32 RowPitch)
{
	UNREFERENCED_VARIABLE(Depth);

	const Uint32 SizeInBytes	= Height * RowPitch;
	const Uint32 NewOffset		= UploadBufferOffset + SizeInBytes;
	if (NewOffset >= UploadBuffer->GetSizeInBytes())
	{
		// Destroy old buffer
		DeferDestruction(UploadBuffer);
		SAFEDELETE(UploadBuffer);

		// Create new one
		CreateUploadBuffer(NewOffset);
	}

	// Copy to GPU buffer
	Byte* Memory = UploadPointer + UploadBufferOffset;
	const Byte* Source = reinterpret_cast<const Byte*>(Src);
	for (Uint32 Y = 0; Y < Height; Y++)
	{
		memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Memory) + Y * RowPitch), Source + (Y * Width * Stride), Width * Stride);
	}

	// Copy to Dest
	D3D12_TEXTURE_COPY_LOCATION SourceLocation = { };
	SourceLocation.pResource							= UploadBuffer->GetResource();
	SourceLocation.Type									= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	SourceLocation.PlacedFootprint.Footprint.Format		= Format;
	SourceLocation.PlacedFootprint.Footprint.Width		= Width;
	SourceLocation.PlacedFootprint.Footprint.Height		= Height;
	SourceLocation.PlacedFootprint.Footprint.Depth		= 1;
	SourceLocation.PlacedFootprint.Footprint.RowPitch	= RowPitch;

	D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
	DestLocation.pResource			= Dest->GetResource();
	DestLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	DestLocation.SubresourceIndex	= 0;
	
	CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

	UploadBufferOffset = NewOffset;
}

void D3D12CommandList::DeferDestruction(D3D12Resource* Resource)
{
	ResourcesPendingRelease.EmplaceBack(Resource->GetResource());
}

void D3D12CommandList::TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
	D3D12_RESOURCE_BARRIER Barrier = { };
	Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource	= Resource->GetResource();
	Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Barrier.Transition.StateBefore	= BeforeState;
	Barrier.Transition.StateAfter	= AfterState;

	DeferredResourceBarriers.PushBack(Barrier);
}

void D3D12CommandList::UnorderedAccessBarrier(D3D12Resource* Resource)
{
	D3D12_RESOURCE_BARRIER Barrier = { };
	Barrier.Type			= D3D12_RESOURCE_BARRIER_TYPE_UAV;
	Barrier.UAV.pResource	= Resource->GetResource();
	
	DeferredResourceBarriers.PushBack(Barrier);
}

void D3D12CommandList::SetDebugName(const std::string& DebugName)
{
	std::wstring WideDebugName = ConvertToWide(DebugName);
	CommandList->SetName(WideDebugName.c_str());
}

bool D3D12CommandList::CreateUploadBuffer(Uint32 SizeInBytes)
{
	BufferProperties UploadBufferProps = { };
	UploadBufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	UploadBufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;
	UploadBufferProps.SizeInBytes	= SizeInBytes;
	UploadBufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;

	UploadBuffer = new D3D12Buffer(Device);
	if (UploadBuffer->Initialize(UploadBufferProps))
	{
		UploadBufferOffset = 0;

		UploadPointer = reinterpret_cast<Byte*>(UploadBuffer->Map());
		return true;
	}
	else
	{
		return false;
	}
}
