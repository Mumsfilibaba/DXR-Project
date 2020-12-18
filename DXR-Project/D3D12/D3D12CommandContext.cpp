#include "D3D12CommandContext.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandList.h"

/*
* D3D12CommandContext
*/

D3D12CommandContext::D3D12CommandContext(D3D12Device* InDevice, D3D12CommandQueue* InCmdQueue, const D3D12DefaultRootSignatures& InDefaultRootSignatures)
	: D3D12DeviceChild(InDevice)
	, CmdQueue(InCmdQueue)
	, CmdAllocators()
	, CmdList(nullptr)
	, Fence(nullptr)
	, DefaultRootSignatures(InDefaultRootSignatures)
{
}

D3D12CommandContext::~D3D12CommandContext()
{
	for (D3D12CommandAllocator* CmdAllocator : CmdAllocators)
	{
		SAFEDELETE(CmdAllocator);
	}

	SAFEDELETE(CmdList);
}

bool D3D12CommandContext::Initialize()
{
	VALIDATE(CmdQueue != nullptr);

	// TODO: Have support for more than 3 commandallocators
	for (Uint32 i = 0; i < 3; i++)
	{
		TUniquePtr<D3D12CommandAllocator> CmdAllocator = TUniquePtr(Device->CreateCommandAllocator(CmdQueue->GetType()));
		if (CmdAllocator)
		{
			CmdAllocators.EmplaceBack(CmdAllocator.Release());
		}
		else
		{
			return false;
		}
	}

	CmdList = Device->CreateCommandList(CmdQueue->GetType(), CmdAllocators[0], nullptr);
	if (!CmdList)
	{
		return false;
	}

	Fence = Device->CreateFence(0);
	if (!CmdList)
	{
		return false;
	}

	return true;
}

void D3D12CommandContext::Begin()
{
	IsReady = true;
}

void D3D12CommandContext::End()
{
	IsReady = false;
}

void D3D12CommandContext::ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor)
{
}

void D3D12CommandContext::ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) 
{
}

void D3D12CommandContext::ClearUnorderedAccessView(UnorderedAccessView* UnorderedAccessView, const ColorClearValue& ClearColor)
{
}

void D3D12CommandContext::BeginRenderPass()
{
}

void D3D12CommandContext::EndRenderPass()
{
}

void D3D12CommandContext::BindViewport(const Viewport& Viewport, Uint32 Slot)
{
}

void D3D12CommandContext::BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot)
{
}

void D3D12CommandContext::BindBlendFactor(const ColorClearValue& Color)
{
}

void D3D12CommandContext::BindPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
{
}

void D3D12CommandContext::BindVertexBuffers(
	VertexBuffer* const * VertexBuffers, 
	Uint32 BufferCount, 
	Uint32 BufferSlot)
{
}

void D3D12CommandContext::BindIndexBuffer(IndexBuffer* IndexBuffer)
{
}

void D3D12CommandContext::BindRayTracingScene(RayTracingScene* RayTracingScene)
{
}

void D3D12CommandContext::BindRenderTargets(
	RenderTargetView* const * RenderTargetViews, 
	Uint32 RenderTargetCount, 
	DepthStencilView* DepthStencilView)
{
}

void D3D12CommandContext::BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState)
{
}

void D3D12CommandContext::BindComputePipelineState(class ComputePipelineState* PipelineState)
{
}

void D3D12CommandContext::BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState)
{
}

void D3D12CommandContext::BindConstantBuffers(
	Shader* Shader, 
	ConstantBuffer* const * ConstantBuffers, 
	Uint32 ConstantBufferCount, 
	Uint32 StartSlot)
{
}

void D3D12CommandContext::BindShaderResourceViews(
	Shader* Shader, 
	ShaderResourceView* const* ShaderResourceViews, 
	Uint32 ShaderResourceViewCount, 
	Uint32 StartSlot)
{
}

void D3D12CommandContext::BindUnorderedAccessViews(
	Shader* Shader, 
	UnorderedAccessView* const* UnorderedAccessViews, 
	Uint32 UnorderedAccessViewCount, 
	Uint32 StartSlot)
{
}

void D3D12CommandContext::ResolveTexture(Texture* Destination, Texture* Source)
{
}

void D3D12CommandContext::UpdateBuffer(
	Buffer* Destination, 
	Uint64 OffsetInBytes, 
	Uint64 SizeInBytes, 
	const VoidPtr SourceData)
{
}

void D3D12CommandContext::UpdateTexture2D(
	Texture2D* Destination, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 MipLevel, 
	const VoidPtr SourceData)
{
}

void D3D12CommandContext::CopyBuffer(
	Buffer* Destination, 
	Buffer* Source, 
	const CopyBufferInfo& CopyInfo)
{
}

void D3D12CommandContext::CopyTexture(
	Texture* Destination, 
	Texture* Source, 
	const CopyTextureInfo& CopyTextureInfo)
{
}

void D3D12CommandContext::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
{
}

void D3D12CommandContext::BuildRayTracingScene(RayTracingScene* RayTracingScene)
{
}

void D3D12CommandContext::GenerateMips(Texture* Texture)
{
	//using namespace Microsoft::WRL;

	//const D3D12_RESOURCE_DESC& Desc = Dest->GetDesc();
	//VALIDATE(Desc.MipLevels > 1);

	//// Check Type
	//bool IsTextureCube = false;
	//if (Dest->GetShaderResourceView(0)->GetDesc().ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
	//{
	//	IsTextureCube = true;
	//}

	//// Create staging texture
	//TextureProperties StagingTextureProps;
	//StagingTextureProps.DebugName = "StagingTexture";
	//StagingTextureProps.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	//StagingTextureProps.Format = Desc.Format;
	//StagingTextureProps.Width = static_cast<Uint16>(Desc.Width);
	//StagingTextureProps.Height = static_cast<Uint16>(Desc.Height);
	//StagingTextureProps.ArrayCount = static_cast<Uint16>(Desc.DepthOrArraySize);
	//StagingTextureProps.MemoryType = Dest->GetMemoryType();
	//StagingTextureProps.InitalState = D3D12_RESOURCE_STATE_COMMON;
	//StagingTextureProps.MipLevels = Desc.MipLevels;
	//StagingTextureProps.OptimizedClearValue = nullptr;
	//StagingTextureProps.SampleCount = 1;

	//TUniquePtr<D3D12Texture> StagingTexture = MakeUnique<D3D12Texture>(Device);
	//if (!StagingTexture->Initialize(StagingTextureProps))
	//{
	//	LOG_ERROR("[D3D12CommandList] Failed to create StagingTexture for GenerateMips");
	//	return;
	//}

	//// Create SRV
	//D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
	//SRVDesc.Format = Desc.Format;
	//SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//if (IsTextureCube)
	//{
	//	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	//	SRVDesc.TextureCube.MipLevels = Desc.MipLevels;
	//	SRVDesc.TextureCube.MostDetailedMip = 0;
	//	SRVDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	//}
	//else
	//{
	//	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//	SRVDesc.Texture2D.MipLevels = Desc.MipLevels;
	//	SRVDesc.Texture2D.MostDetailedMip = 0;
	//}

	//StagingTexture->SetShaderResourceView(MakeShared<D3D12ShaderResourceView>(Device, StagingTexture->GetResource(), &SRVDesc), 0);

	//// Create UAVs
	//D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = { };
	//UAVDesc.Format = Desc.Format;
	//if (IsTextureCube)
	//{
	//	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	//	UAVDesc.Texture2DArray.ArraySize = 6;
	//	UAVDesc.Texture2DArray.FirstArraySlice = 0;
	//	UAVDesc.Texture2DArray.PlaneSlice = 0;
	//}
	//else
	//{
	//	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	//	UAVDesc.Texture2D.PlaneSlice = 0;
	//}

	//for (Uint32 i = 0; i < Desc.MipLevels; i++)
	//{
	//	if (IsTextureCube)
	//	{
	//		UAVDesc.Texture2DArray.MipSlice = i;
	//	}
	//	else
	//	{
	//		UAVDesc.Texture2D.MipSlice = i;
	//	}

	//	StagingTexture->SetUnorderedAccessView(MakeShared<D3D12UnorderedAccessView>(Device, nullptr, StagingTexture->GetResource(), &UAVDesc), i);
	//}

	//// Create PSO and RS
	//ComPtr<ID3D12PipelineState> PipelineState;
	//if (IsTextureCube)
	//{
	//	if (!MipGenHelper.GenerateMipsTexCube_PSO)
	//	{
	//		Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", "Main", "cs_6_0");
	//		if (!CSBlob)
	//		{
	//			return;
	//		}

	//		ComputePipelineStateProperties GenMipsProperties = { };
	//		GenMipsProperties.DebugName = "Generate MipLevels Pipeline TexCube";
	//		GenMipsProperties.RootSignature = nullptr;
	//		GenMipsProperties.CSBlob = CSBlob.Get();

	//		MipGenHelper.GenerateMipsTexCube_PSO = MakeUnique<D3D12ComputePipelineState>(Device);
	//		if (!MipGenHelper.GenerateMipsTexCube_PSO->Initialize(GenMipsProperties))
	//		{
	//			return;
	//		}

	//		// Create rootsignature
	//		if (!MipGenHelper.GenerateMipsRootSignature)
	//		{
	//			MipGenHelper.GenerateMipsRootSignature = MakeUnique<D3D12RootSignature>(Device);
	//			if (MipGenHelper.GenerateMipsRootSignature->Initialize(CSBlob.Get()))
	//			{
	//				MipGenHelper.GenerateMipsRootSignature->SetDebugName("Generate MipLevels RootSignature");
	//			}
	//			else
	//			{
	//				return;
	//			}
	//		}
	//	}

	//	PipelineState = MipGenHelper.GenerateMipsTexCube_PSO->GetPipeline();
	//}
	//else
	//{
	//	if (!MipGenHelper.GenerateMipsTex2D_PSO)
	//	{
	//		Microsoft::WRL::ComPtr<IDxcBlob> CSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", "Main", "cs_6_0");
	//		if (!CSBlob)
	//		{
	//			return;
	//		}

	//		ComputePipelineStateProperties GenMipsProperties = { };
	//		GenMipsProperties.DebugName = "Generate MipLevels Pipeline Tex2D";
	//		GenMipsProperties.RootSignature = nullptr;
	//		GenMipsProperties.CSBlob = CSBlob.Get();

	//		MipGenHelper.GenerateMipsTex2D_PSO = MakeUnique<D3D12ComputePipelineState>(Device);
	//		if (!MipGenHelper.GenerateMipsTex2D_PSO->Initialize(GenMipsProperties))
	//		{
	//			return;
	//		}

	//		// Create rootsignature
	//		if (!MipGenHelper.GenerateMipsRootSignature)
	//		{
	//			MipGenHelper.GenerateMipsRootSignature = MakeUnique<D3D12RootSignature>(Device);
	//			if (MipGenHelper.GenerateMipsRootSignature->Initialize(CSBlob.Get()))
	//			{
	//				MipGenHelper.GenerateMipsRootSignature->SetDebugName("Generate MipLevels RootSignature");
	//			}
	//			else
	//			{
	//				return;
	//			}
	//		}
	//	}

	//	PipelineState = MipGenHelper.GenerateMipsTex2D_PSO->GetPipeline();
	//}

	//// Create Resources for generating Miplevels
	//const Uint32 MipLevelsPerDispatch = 4;
	//Uint32 NumDispatches = Desc.MipLevels / MipLevelsPerDispatch;

	//Uint32 MiplevelsLastDispatch = Desc.MipLevels - (MipLevelsPerDispatch * NumDispatches);
	//if (MiplevelsLastDispatch > 0)
	//{
	//	if (!MipGenHelper.NULLView)
	//	{
	//		UAVDesc.Format = Desc.Format;
	//		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	//		UAVDesc.Texture2D.MipSlice = 0;
	//		UAVDesc.Texture2D.PlaneSlice = 0;
	//		MipGenHelper.NULLView = MakeUnique<D3D12UnorderedAccessView>(Device, nullptr, nullptr, &UAVDesc);
	//	}

	//	NumDispatches++;
	//}

	//// Resize if necessary
	//if (MipGenHelper.UAVDescriptorTables.Size() < NumDispatches)
	//{
	//	MipGenHelper.UAVDescriptorTables.Resize(NumDispatches);
	//}

	//// Bind ShaderResourceView
	//if (!MipGenHelper.SRVDescriptorTable)
	//{
	//	MipGenHelper.SRVDescriptorTable = MakeUnique<D3D12DescriptorTable>(Device, 1);
	//}

	//MipGenHelper.SRVDescriptorTable->SetShaderResourceView(StagingTexture->GetShaderResourceView(0).Get(), 0);
	//MipGenHelper.SRVDescriptorTable->CopyDescriptors();

	//// Bind UnorderedAccessViews
	//Uint32 UAVIndex = 0;
	//for (Uint32 i = 0; i < NumDispatches; i++)
	//{
	//	if (!MipGenHelper.UAVDescriptorTables[i])
	//	{
	//		MipGenHelper.UAVDescriptorTables[i] = MakeUnique<D3D12DescriptorTable>(Device, 4);
	//	}

	//	for (Uint32 j = 0; j < MipLevelsPerDispatch; j++)
	//	{
	//		if (UAVIndex < Desc.MipLevels)
	//		{
	//			MipGenHelper.UAVDescriptorTables[i]->SetUnorderedAccessView(StagingTexture->GetUnorderedAccessView(UAVIndex).Get(), j);
	//			UAVIndex++;
	//		}
	//		else
	//		{
	//			MipGenHelper.UAVDescriptorTables[i]->SetUnorderedAccessView(MipGenHelper.NULLView.Get(), j);
	//		}
	//	}

	//	MipGenHelper.UAVDescriptorTables[i]->CopyDescriptors();
	//}

	//// Copy the source over to the staging texture
	//TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
	//TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	//CopyResource(StagingTexture.Get(), Dest);

	//TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//SetPipelineState(PipelineState.Get());
	//SetComputeRootSignature(MipGenHelper.GenerateMipsRootSignature->GetRootSignature());

	//ID3D12DescriptorHeap* GlobalHeap = Device->GetGlobalOnlineResourceHeap()->GetHeap();
	//SetDescriptorHeaps(&GlobalHeap, 1);
	//SetComputeRootDescriptorTable(MipGenHelper.SRVDescriptorTable->GetGPUTableStartHandle(), 1);

	//struct ConstantBuffer
	//{
	//	Uint32		SrcMipLevel;
	//	Uint32		NumMipLevels;
	//	XMFLOAT2	TexelSize;
	//} CB0;

	//Uint32 DstWidth = static_cast<Uint32>(Desc.Width);
	//Uint32 DstHeight = Desc.Height;
	//CB0.SrcMipLevel = 0;

	//const Uint32 ThreadsZ = IsTextureCube ? 6 : 1;
	//Uint32 RemainingMiplevels = Desc.MipLevels;
	//for (Uint32 i = 0; i < NumDispatches; i++)
	//{
	//	CB0.TexelSize = XMFLOAT2(1.0f / static_cast<Float32>(DstWidth), 1.0f / static_cast<Float32>(DstHeight));
	//	CB0.NumMipLevels = std::min<Uint32>(4, RemainingMiplevels);

	//	SetComputeRoot32BitConstants(&CB0, 4, 0, 0);
	//	SetComputeRootDescriptorTable(MipGenHelper.UAVDescriptorTables[i]->GetGPUTableStartHandle(), 2);

	//	const Uint32 ThreadsX = DivideByMultiple(DstWidth, 8);
	//	const Uint32 ThreadsY = DivideByMultiple(DstHeight, 8);
	//	Dispatch(ThreadsX, ThreadsY, ThreadsZ);

	//	UnorderedAccessBarrier(StagingTexture.Get());

	//	DstWidth = DstWidth / 16;
	//	DstHeight = DstHeight / 16;

	//	CB0.SrcMipLevel += 3;
	//	RemainingMiplevels -= MipLevelsPerDispatch;
	//}

	//TransitionBarrier(Dest, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	//TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);

	//CopyResource(Dest, StagingTexture.Get());

	//DeferDestruction(StagingTexture.Get());
}

void D3D12CommandContext::TransitionTexture(
	Texture* Texture, 
	EResourceState BeforeState, 
	EResourceState AfterState)
{
}

void D3D12CommandContext::TransitionBuffer(
	Buffer* Buffer, 
	EResourceState BeforeState, 
	EResourceState AfterState)
{
}

void D3D12CommandContext::UnorderedAccessTextureBarrier(Texture* Texture)
{
}

void D3D12CommandContext::Draw(Uint32 VertexCount, Uint32 StartVertexLocation)
{
}

void D3D12CommandContext::DrawIndexed(
	Uint32 IndexCount, 
	Uint32 StartIndexLocation, 
	Uint32 BaseVertexLocation)
{
}

void D3D12CommandContext::DrawInstanced(
	Uint32 VertexCountPerInstance, 
	Uint32 InstanceCount, 
	Uint32 StartVertexLocation, 
	Uint32 StartInstanceLocation)
{
}

void D3D12CommandContext::DrawIndexedInstanced(
	Uint32 IndexCountPerInstance, 
	Uint32 InstanceCount, 
	Uint32 StartIndexLocation, 
	Uint32 BaseVertexLocation, 
	Uint32 StartInstanceLocation)
{
}

void D3D12CommandContext::Dispatch(
	Uint32 WorkGroupsX, 
	Uint32 WorkGroupsY, 
	Uint32 WorkGroupsZ)
{
}

void D3D12CommandContext::DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth)
{
}

void D3D12CommandContext::Flush()
{
}
