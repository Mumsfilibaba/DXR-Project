#include "D3D12CommandContext.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandList.h"
#include "D3D12Helpers.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12PipelineState.h"

/*
* D3D12CommandContext
*/

D3D12CommandContext::D3D12CommandContext(D3D12Device* InDevice, TSharedRef<D3D12CommandQueue>& InCmdQueue, const D3D12DefaultRootSignatures& InDefaultRootSignatures)
	: ICommandContext()
	, D3D12DeviceChild(InDevice)
	, CmdQueue(InCmdQueue)
	, CmdList(nullptr)
	, Fence(nullptr)
	, CmdBatches()
	, DefaultRootSignatures(InDefaultRootSignatures)
	, VertexBufferState()
	, RenderTargetState()
	, BarrierBatcher()
	, GenerateMipsHelper()
{
}

D3D12CommandContext::~D3D12CommandContext()
{
	Flush();
}

bool D3D12CommandContext::Initialize()
{
	VALIDATE(CmdQueue != nullptr);

	// TODO: Have support for more than 3 commandallocators
	for (UInt32 i = 0; i < 3; i++)
	{
		TSharedPtr<D3D12CommandAllocator> CmdAllocator = TSharedPtr(Device->CreateCommandAllocator(CmdQueue->GetType()));
		if (!CmdAllocator)
		{
			return false;
		}
		
		// TODO: Fix this 1024 is good for now
		TSharedPtr<D3D12OnlineDescriptorHeap> OnlineHeap = TSharedPtr(new D3D12OnlineDescriptorHeap(Device, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		if (!CmdAllocator)
		{
			return false;
		}

		CmdBatches.EmplaceBack(CmdAllocator, OnlineHeap);
	}

	CmdList = Device->CreateCommandList(CmdQueue->GetType(), CmdBatches[0].GetAllocator(), nullptr);
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
	VALIDATE(IsReady == false);

	CmdBatch = &CmdBatches[NextCmdAllocator];
	NextCmdAllocator = (NextCmdAllocator + 1) % CmdBatches.Size();

	if (FenceValue >= CmdBatches.Size())
	{
		const UInt64 WaitValue = Math::Max(FenceValue - (CmdBatches.Size() - 1), 0ULL);
		Fence->WaitForValue(WaitValue);
	}

	if (!CmdBatch->Reset())
	{
		return;
	}

	ClearState();

	if (!CmdList->Reset(CmdBatch->GetAllocator()))
	{
		return;
	}

	IsReady = true;
}

void D3D12CommandContext::End()
{
	VALIDATE(IsReady == true);

	BarrierBatcher.FlushBarriers(*CmdList);

	if (!CmdList->Close())
	{
		return;
	}

	CmdQueue->ExecuteCommandList(CmdList.Get());

	const UInt64 CurrentFenceValue = ++FenceValue;
	if (!CmdQueue->SignalFence(Fence.Get(), CurrentFenceValue))
	{
		return;
	}

	// Reset state
	CmdBatch	= nullptr;
	IsReady		= false;
}

void D3D12CommandContext::ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor)
{
	VALIDATE(RenderTargetView != nullptr);

	D3D12RenderTargetView* View = static_cast<D3D12RenderTargetView*>(RenderTargetView);
	BarrierBatcher.FlushBarriers(*CmdList);
	CmdList->ClearRenderTargetView(View->GetOfflineHandle(), ClearColor.RGBA, 0, nullptr);
}

void D3D12CommandContext::ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) 
{
	VALIDATE(DepthStencilView != nullptr);

	D3D12DepthStencilView* View = static_cast<D3D12DepthStencilView*>(DepthStencilView);
	BarrierBatcher.FlushBarriers(*CmdList);
	CmdList->ClearDepthStencilView(
		View->GetOfflineHandle(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		ClearValue.Depth, 
		ClearValue.Stencil);
}

void D3D12CommandContext::ClearUnorderedAccessViewFloat(UnorderedAccessView* UnorderedAccessView, const Float ClearColor[4])
{
	D3D12UnorderedAccessView* View = static_cast<D3D12UnorderedAccessView*>(UnorderedAccessView);
	BarrierBatcher.FlushBarriers(*CmdList);

	const D3D12_GPU_DESCRIPTOR_HANDLE Descriptor = CmdBatch->AllocateResourceDescriptors(1);
	CmdList->ClearUnorderedAccessViewFloat(Descriptor, View, ClearColor);
}

void D3D12CommandContext::BeginRenderPass()
{
	// Empty for now
}

void D3D12CommandContext::EndRenderPass()
{
	// Empty for now
}

void D3D12CommandContext::BindViewport(
	Float Width, 
	Float Height, 
	Float MinDepth, 
	Float MaxDepth, 
	Float x, 
	Float y)
{
	D3D12_VIEWPORT Viewport;
	Viewport.Width		= Width;
	Viewport.Height		= Height;
	Viewport.MaxDepth	= MaxDepth;
	Viewport.MinDepth	= MinDepth;
	Viewport.TopLeftX	= x;
	Viewport.TopLeftY	= y;

	CmdList->RSSetViewports(&Viewport, 1);
}

void D3D12CommandContext::BindScissorRect(
	Float Width, 
	Float Height, 
	Float x, 
	Float y)
{
	D3D12_RECT ScissorRect;
	ScissorRect.top		= y;
	ScissorRect.bottom	= Height;
	ScissorRect.left	= x;
	ScissorRect.right	= Width;

	CmdList->RSSetScissorRects(&ScissorRect, 1);
}

void D3D12CommandContext::BindBlendFactor(const ColorClearValue& Color)
{
	CmdList->OMSetBlendFactor(Color.RGBA);
}

void D3D12CommandContext::BindPrimitiveTopology(EPrimitiveTopology InPrimitveTopology)
{
	const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = ConvertPrimitiveTopology(InPrimitveTopology);
	CmdList->IASetPrimitiveTopology(PrimitiveTopology);
}

void D3D12CommandContext::BindVertexBuffers(
	VertexBuffer* const * VertexBuffers, 
	UInt32 BufferCount, 
	UInt32 BufferSlot)
{
	for (UInt32 i = 0; i < BufferCount; i++)
	{
		D3D12VertexBuffer* DxVertexBuffer = static_cast<D3D12VertexBuffer*>(VertexBuffers[i]);
		VertexBufferState.BindVertexBuffer(DxVertexBuffer, BufferSlot + i);
	}
	
	CmdList->IASetVertexBuffers(
		0, 
		VertexBufferState.GetVertexBufferViews(),
		VertexBufferState.GetNumVertexBufferViews());
}

void D3D12CommandContext::BindIndexBuffer(IndexBuffer* IndexBuffer)
{
	if (IndexBuffer)
	{
		D3D12IndexBuffer* DxIndexBuffer = static_cast<D3D12IndexBuffer*>(IndexBuffer);
		CmdList->IASetIndexBuffer(&DxIndexBuffer->GetView());
	}
	else
	{
		CmdList->IASetIndexBuffer(nullptr);
	}
}

void D3D12CommandContext::BindRayTracingScene(RayTracingScene* RayTracingScene)
{
	// TODO: Implement this function
}

void D3D12CommandContext::BindRenderTargets(
	RenderTargetView* const* RenderTargetViews, 
	UInt32 RenderTargetCount, 
	DepthStencilView* DepthStencilView)
{
	for (UInt32 i = 0; i < RenderTargetCount; i++)
	{
		D3D12RenderTargetView* DxRenderTargetView = static_cast<D3D12RenderTargetView*>(RenderTargetViews[i]);
		RenderTargetState.BindRenderTargetView(DxRenderTargetView, i);
	}

	D3D12DepthStencilView* DxDepthStencilView = static_cast<D3D12DepthStencilView*>(DepthStencilView);
	RenderTargetState.BindDepthStencilView(DxDepthStencilView);

	CmdList->OMSetRenderTargets(
		RenderTargetState.GetRenderTargetViewHandles(), 
		RenderTargetState.GetNumRenderTargetViewHandles(),
		FALSE, 
		RenderTargetState.GetDepthStencilHandle());
}

void D3D12CommandContext::BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState)
{
	VALIDATE(PipelineState != nullptr);

	D3D12GraphicsPipelineState* DxPipelineState = static_cast<D3D12GraphicsPipelineState*>(PipelineState);
	CmdList->SetGraphicsRootSignature(DxPipelineState->GetRootSignature());
	CmdList->SetPipelineState(DxPipelineState->GetPipeline());
}

void D3D12CommandContext::BindComputePipelineState(class ComputePipelineState* PipelineState)
{
	VALIDATE(PipelineState != nullptr);

	D3D12ComputePipelineState* DxPipelineState = static_cast<D3D12ComputePipelineState*>(PipelineState);
	CmdList->SetComputeRootSignature(DxPipelineState->GetRootSignature());
	CmdList->SetPipelineState(DxPipelineState->GetPipeline());
}

void D3D12CommandContext::BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState)
{
}

void D3D12CommandContext::VSBindShaderResourceViews(
	ShaderResourceView* const* ShaderResourceViews, 
	UInt32 ShaderResourceViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::VSBindUnorderedAccessViews(
	UnorderedAccessView* const* UnorderedAccessViews, 
	UInt32 UnorderedAccessViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::VSBindConstantBuffers(
	ConstantBuffer* const* ConstantBuffers, 
	UInt32 ConstantBufferCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::HSBindShaderResourceViews(
	ShaderResourceView* const* ShaderResourceViews, 
	UInt32 ShaderResourceViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::HSBindUnorderedAccessViews(
	UnorderedAccessView* const* UnorderedAccessViews, 
	UInt32 UnorderedAccessViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::HSBindConstantBuffers(
	ConstantBuffer* const* ConstantBuffers, 
	UInt32 ConstantBufferCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::DSBindShaderResourceViews(
	ShaderResourceView* const* ShaderResourceViews,
	UInt32 ShaderResourceViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::DSBindUnorderedAccessViews(
	UnorderedAccessView* const* UnorderedAccessViews, 
	UInt32 UnorderedAccessViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::DSBindConstantBuffers(
	ConstantBuffer* const* ConstantBuffers, 
	UInt32 ConstantBufferCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::GSBindShaderResourceViews(
	ShaderResourceView* const* ShaderResourceViews, 
	UInt32 ShaderResourceViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::GSBindUnorderedAccessViews(
	UnorderedAccessView* const* UnorderedAccessViews, 
	UInt32 UnorderedAccessViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::GSBindConstantBuffers(
	ConstantBuffer* const* ConstantBuffers,
	UInt32 ConstantBufferCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::PSBindShaderResourceViews(
	ShaderResourceView* const* ShaderResourceViews, 
	UInt32 ShaderResourceViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::PSBindUnorderedAccessViews(
	UnorderedAccessView* const* UnorderedAccessViews, 
	UInt32 UnorderedAccessViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::PSBindConstantBuffers(
	ConstantBuffer* const* ConstantBuffers, 
	UInt32 ConstantBufferCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::CSBindShaderResourceViews(
	ShaderResourceView* const* ShaderResourceViews,
	UInt32 ShaderResourceViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::CSBindUnorderedAccessViews(
	UnorderedAccessView* const* UnorderedAccessViews,
	UInt32 UnorderedAccessViewCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::CSBindConstantBuffers(
	ConstantBuffer* const* ConstantBuffers, 
	UInt32 ConstantBufferCount, 
	UInt32 StartSlot)
{
}

void D3D12CommandContext::ResolveTexture(Texture* Destination, Texture* Source)
{
	//TODO: For now texture must be the same format. I.e typeless does probably not work

	D3D12Texture* DxDestination	= D3D12TextureCast(Destination);
	D3D12Texture* DxSource		= D3D12TextureCast(Source);
	const DXGI_FORMAT DstFormat	= DxDestination->GetDxgiFormat();
	const DXGI_FORMAT SrcFormat	= DxSource->GetDxgiFormat();
	
	VALIDATE(DstFormat == SrcFormat);

	BarrierBatcher.FlushBarriers(*CmdList);
	CmdList->ResolveSubresource(DxDestination, DxSource, DstFormat);
}

void D3D12CommandContext::UpdateBuffer(
	Buffer* Destination, 
	UInt64 OffsetInBytes, 
	UInt64 SizeInBytes, 
	const Void* SourceData)
{
	// TODO: Implement this function
	BarrierBatcher.FlushBarriers(*CmdList);
}

void D3D12CommandContext::UpdateTexture2D(
	Texture2D* Destination, 
	UInt32 Width, 
	UInt32 Height, 
	UInt32 MipLevel, 
	const Void* SourceData)
{
	// TODO: Implement this function
	BarrierBatcher.FlushBarriers(*CmdList);
}

void D3D12CommandContext::CopyBuffer(
	Buffer* Destination, 
	Buffer* Source, 
	const CopyBufferInfo& CopyInfo)
{
	D3D12Resource* DxDestination	= D3D12BufferCast(Destination);
	D3D12Resource* DxSource			= D3D12BufferCast(Source);
	
	BarrierBatcher.FlushBarriers(*CmdList);
	CmdList->CopyBuffer(
		DxDestination, 
		CopyInfo.DestinationOffset, 
		DxSource,
		CopyInfo.SourceOffset,
		CopyInfo.SizeInBytes);
}

void D3D12CommandContext::CopyTexture(Texture* Destination, Texture* Source)
{
	D3D12Texture* DxDestination	= D3D12TextureCast(Destination);
	D3D12Texture* DxSource		= D3D12TextureCast(Source);

	BarrierBatcher.FlushBarriers(*CmdList);
	CmdList->CopyResource(DxDestination, DxSource);
}

void D3D12CommandContext::CopyTextureRegion(
	Texture* Destination, 
	Texture* Source, 
	const CopyTextureInfo& CopyInfo)
{
	D3D12Texture* DxDestination = D3D12TextureCast(Destination);
	D3D12Texture* DxSource		= D3D12TextureCast(Source);

	// Source
	D3D12_TEXTURE_COPY_LOCATION SourceLocation;
	Memory::Memzero(&SourceLocation, sizeof(D3D12_TEXTURE_COPY_LOCATION));

	SourceLocation.pResource		= DxSource->GetResource();
	SourceLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	SourceLocation.SubresourceIndex	= CopyInfo.Source.SubresourceIndex;

	D3D12_BOX SourceBox;
	SourceBox.left		= CopyInfo.Source.x;
	SourceBox.right		= CopyInfo.Source.x + CopyInfo.Width;
	SourceBox.bottom	= CopyInfo.Source.y;
	SourceBox.top		= CopyInfo.Source.y + CopyInfo.Height;
	SourceBox.front		= CopyInfo.Source.z;
	SourceBox.back		= CopyInfo.Source.z + CopyInfo.Depth;

	// Destination
	D3D12_TEXTURE_COPY_LOCATION DestinationLocation;
	Memory::Memzero(&DestinationLocation, sizeof(D3D12_TEXTURE_COPY_LOCATION));

	DestinationLocation.pResource			= DxDestination->GetResource();
	DestinationLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	DestinationLocation.SubresourceIndex	= CopyInfo.Destination.SubresourceIndex;

	BarrierBatcher.FlushBarriers(*CmdList);

	CmdList->CopyTextureRegion(
		&DestinationLocation,
		CopyInfo.Destination.x,
		CopyInfo.Destination.y,
		CopyInfo.Destination.z,
		&SourceLocation,
		&SourceBox);
}

void D3D12CommandContext::DestroyResource(PipelineResource* Resource)
{
	CmdBatch->EnqueueResourceDestruction(Resource);
}

void D3D12CommandContext::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
{
	// TODO: Implement this function
	BarrierBatcher.FlushBarriers(*CmdList);
}

void D3D12CommandContext::BuildRayTracingScene(RayTracingScene* RayTracingScene)
{
	// TODO: Implement this function
	BarrierBatcher.FlushBarriers(*CmdList);
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

	//for (UInt32 i = 0; i < Desc.MipLevels; i++)
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
	//const UInt32 MipLevelsPerDispatch = 4;
	//UInt32 NumDispatches = Desc.MipLevels / MipLevelsPerDispatch;

	//UInt32 MiplevelsLastDispatch = Desc.MipLevels - (MipLevelsPerDispatch * NumDispatches);
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
	//UInt32 UAVIndex = 0;
	//for (UInt32 i = 0; i < NumDispatches; i++)
	//{
	//	if (!MipGenHelper.UAVDescriptorTables[i])
	//	{
	//		MipGenHelper.UAVDescriptorTables[i] = MakeUnique<D3D12DescriptorTable>(Device, 4);
	//	}

	//	for (UInt32 j = 0; j < MipLevelsPerDispatch; j++)
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
	//	UInt32		SrcMipLevel;
	//	UInt32		NumMipLevels;
	//	XMFLOAT2	TexelSize;
	//} CB0;

	//UInt32 DstWidth = static_cast<UInt32>(Desc.Width);
	//UInt32 DstHeight = Desc.Height;
	//CB0.SrcMipLevel = 0;

	//const UInt32 ThreadsZ = IsTextureCube ? 6 : 1;
	//UInt32 RemainingMiplevels = Desc.MipLevels;
	//for (UInt32 i = 0; i < NumDispatches; i++)
	//{
	//	CB0.TexelSize = XMFLOAT2(1.0f / static_cast<Float32>(DstWidth), 1.0f / static_cast<Float32>(DstHeight));
	//	CB0.NumMipLevels = std::min<UInt32>(4, RemainingMiplevels);

	//	SetComputeRoot32BitConstants(&CB0, 4, 0, 0);
	//	SetComputeRootDescriptorTable(MipGenHelper.UAVDescriptorTables[i]->GetGPUTableStartHandle(), 2);

	//	const UInt32 ThreadsX = DivideByMultiple(DstWidth, 8);
	//	const UInt32 ThreadsY = DivideByMultiple(DstHeight, 8);
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
	const D3D12_RESOURCE_STATES DxBeforeState	= ConvertResourceState(BeforeState);
	const D3D12_RESOURCE_STATES DxAfterState	= ConvertResourceState(AfterState);

	D3D12Resource* Resource = D3D12TextureCast(Texture);
	BarrierBatcher.AddTransitionBarrier(Resource, DxBeforeState, DxAfterState);
}

void D3D12CommandContext::TransitionBuffer(
	Buffer* Buffer, 
	EResourceState BeforeState, 
	EResourceState AfterState)
{
	const D3D12_RESOURCE_STATES DxBeforeState	= ConvertResourceState(BeforeState);
	const D3D12_RESOURCE_STATES DxAfterState	= ConvertResourceState(AfterState);
	
	D3D12Resource* Resource = D3D12BufferCast(Buffer);
	BarrierBatcher.AddTransitionBarrier(Resource, DxBeforeState, DxAfterState);
}

void D3D12CommandContext::UnorderedAccessTextureBarrier(Texture* Texture)
{
	D3D12Resource* Resource = D3D12TextureCast(Texture);
	BarrierBatcher.AddUnorderedAccessBarrier(Resource);
}

void D3D12CommandContext::Draw(UInt32 VertexCount, UInt32 StartVertexLocation)
{
	BarrierBatcher.FlushBarriers(*CmdList);
	//CmdList->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void D3D12CommandContext::DrawIndexed(
	UInt32 IndexCount, 
	UInt32 StartIndexLocation, 
	UInt32 BaseVertexLocation)
{
	BarrierBatcher.FlushBarriers(*CmdList);
	//CmdList->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void D3D12CommandContext::DrawInstanced(
	UInt32 VertexCountPerInstance, 
	UInt32 InstanceCount, 
	UInt32 StartVertexLocation, 
	UInt32 StartInstanceLocation)
{
	BarrierBatcher.FlushBarriers(*CmdList);
	//CmdList->DrawInstanced(
	//	VertexCountPerInstance,
	//	InstanceCount, 
	//	StartVertexLocation,
	//	StartInstanceLocation);
}

void D3D12CommandContext::DrawIndexedInstanced(
	UInt32 IndexCountPerInstance, 
	UInt32 InstanceCount, 
	UInt32 StartIndexLocation, 
	UInt32 BaseVertexLocation, 
	UInt32 StartInstanceLocation)
{
	BarrierBatcher.FlushBarriers(*CmdList);
	//CmdList->DrawIndexedInstanced(
	//	IndexCountPerInstance, 
	//	InstanceCount, 
	//	StartIndexLocation, 
	//	BaseVertexLocation, 
	//	StartInstanceLocation);
}

void D3D12CommandContext::Dispatch(
	UInt32 ThreadGroupCountX, 
	UInt32 ThreadGroupCountY,
	UInt32 ThreadGroupCountZ)
{
	BarrierBatcher.FlushBarriers(*CmdList);
	//CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void D3D12CommandContext::DispatchRays(UInt32 Width, UInt32 Height, UInt32 Depth)
{
	// TODO: Finish this function
	VALIDATE(false);

	D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
	Memory::Memzero(&RayDispatchDesc, sizeof(D3D12_DISPATCH_RAYS_DESC));

	RayDispatchDesc.Width	= Width;
	RayDispatchDesc.Height	= Height;
	RayDispatchDesc.Depth	= Depth;

	BarrierBatcher.FlushBarriers(*CmdList);
	CmdList->DispatchRays(&RayDispatchDesc);
}

void D3D12CommandContext::ClearState()
{
	VertexBufferState.Reset();
	RenderTargetState.Reset();
}

void D3D12CommandContext::Flush()
{
	BarrierBatcher.FlushBarriers(*CmdList);

	// TODO: Wait for all pending commans that are being executed on the GPU
	Fence->WaitForValue(FenceValue);	
}
