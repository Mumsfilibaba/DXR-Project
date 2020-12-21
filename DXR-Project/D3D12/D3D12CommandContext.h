#pragma once
#include "RenderingCore/ICommandContext.h"

#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Buffer.h"

class D3D12CommandQueue;
class D3D12CommandAllocator;

/*
* D3D12GenerateMipsHelper
*/

struct D3D12GenerateMipsHelper
{
	TSharedRef<class D3D12ComputePipelineState>		GenerateMipsTex2D_PSO;
	TSharedRef<class D3D12ComputePipelineState>		GenerateMipsTexCube_PSO;
	TSharedRef<class D3D12RootSignature>			GenerateMipsRootSignature;
	TUniquePtr<class D3D12DescriptorTable>			SRVDescriptorTable;
	TArray<TUniquePtr<class D3D12DescriptorTable>>	UAVDescriptorTables;
	TSharedRef<class D3D12UnorderedAccessView>		NULLView;
};

/*
* D3D12VertexBufferState
*/

class D3D12VertexBufferState
{
public:
	inline D3D12VertexBufferState()
		: VertexBufferViews()
	{
	}

	FORCEINLINE void BindVertexBuffer(D3D12VertexBuffer* VertexBuffer, UInt32 Slot)
	{
		if (Slot >= VertexBufferViews.Size())
		{
			VertexBufferViews.Resize(Slot + 1);
		}

		// TODO: Maybe save a ref so that we can ensure that the buffer does not get deleted until commandbatch is finished
		VALIDATE(VertexBuffer != nullptr);
		VertexBufferViews[Slot] = VertexBuffer->GetView();
	}

	FORCEINLINE void Reset()
	{
		VertexBufferViews.Clear();
	}

	FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferViews() const
	{
		return VertexBufferViews.Data();
	}

	FORCEINLINE UInt32 GetNumVertexBufferViews() const
	{
		return VertexBufferViews.Size();
	}

private:
	TArray<D3D12_VERTEX_BUFFER_VIEW> VertexBufferViews;
};

/*
* D3D12RenderTargetState
*/

class D3D12RenderTargetState
{
public:
	inline D3D12RenderTargetState()
		: RenderTargetViewHandles()
		, DepthStencilViewHandle({0})
	{
	}

	FORCEINLINE void BindRenderTargetView(
		D3D12RenderTargetView* RenderTargetView, 
		UInt32 Slot)
	{
		if (Slot >= RenderTargetViewHandles.Size())
		{
			RenderTargetViewHandles.Resize(Slot + 1);
		}

		if (RenderTargetView)
		{
			RenderTargetViewHandles[Slot] = RenderTargetView->GetOfflineHandle();
		}
		else
		{
			RenderTargetViewHandles[Slot] = {0};
		}
	}

	FORCEINLINE void BindDepthStencilView(D3D12DepthStencilView* DepthStencilView)
	{
		if (DepthStencilView)
		{
			DepthStencilViewHandle = DepthStencilView->GetOfflineHandle();
		}
		else
		{
			DepthStencilViewHandle = {0};
		}
	}

	FORCEINLINE void Reset()
	{
		RenderTargetViewHandles.Clear();
		DepthStencilViewHandle = {0};
	}

	FORCEINLINE const D3D12_CPU_DESCRIPTOR_HANDLE* GetRenderTargetViewHandles() const
	{
		return RenderTargetViewHandles.Data();
	}

	FORCEINLINE UInt32 GetNumRenderTargetViewHandles() const
	{
		return RenderTargetViewHandles.Size();
	}

	FORCEINLINE const D3D12_CPU_DESCRIPTOR_HANDLE* GetDepthStencilHandle() const
	{
		if (DepthStencilViewHandle.ptr != 0)
		{
			return &DepthStencilViewHandle;
		}
		else
		{
			return nullptr;
		}
	}

private:
	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> RenderTargetViewHandles;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilViewHandle;
};

/*
* D3D12ShaderStageDescriptorState
*/

class D3D12ShaderStageDescriptorState
{
public:
	inline D3D12ShaderStageDescriptorState()
		: ConstantBuffers()
		, ShaderResources()
		, UnorderedAccessResources()
	{
	}

private:
	TArray<D3D12_GPU_DESCRIPTOR_HANDLE> ConstantBuffers;
	TArray<D3D12_GPU_DESCRIPTOR_HANDLE> ShaderResources;
	TArray<D3D12_GPU_DESCRIPTOR_HANDLE> UnorderedAccessResources;
};

/*
* D3D12ShaderDescriptorState
*/

class D3D12ShaderDescriptorState
{
public:
	inline D3D12ShaderDescriptorState()
		: VertexShader()
		, HullShader()
		, DomainShader()
		, GeometryShader()
		, PixelShader()
		, ComputeShader()
	{
	}

private:
	// VS Graphics Pipeline
	D3D12ShaderStageDescriptorState VertexShader;
	D3D12ShaderStageDescriptorState HullShader;
	D3D12ShaderStageDescriptorState DomainShader;
	D3D12ShaderStageDescriptorState GeometryShader;
	D3D12ShaderStageDescriptorState PixelShader;
	// Compute Pipeline
	D3D12ShaderStageDescriptorState ComputeShader;
};

/*
* D3D12CommandBatch
*/

class D3D12CommandBatch
{
public:
	inline D3D12CommandBatch(TSharedPtr<D3D12CommandAllocator>& InCmdAllocator, TSharedPtr<D3D12OnlineDescriptorHeap>& InOnlineHeap)
		: CmdAllocator(InCmdAllocator)
		, OnlineHeap(InOnlineHeap)
		, Resources()
	{
	}

	FORCEINLINE bool Reset()
	{
		if (CmdAllocator->Reset())
		{
			Resources.Clear();
			return true;
		}
		else
		{
			return false;
		}
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE AllocateResourceDescriptors(UInt32 NumDescriptors)
	{
		const UInt32 FirstDescriptor = OnlineHeap->AllocateSlots(NumDescriptors);
		return OnlineHeap->GetGPUSlotAt(FirstDescriptor);
	}

	FORCEINLINE void EnqueueResourceDestruction(PipelineResource* InResource)
	{
		Resources.EmplaceBack(MakeSharedRef<PipelineResource>(InResource));
	}

	FORCEINLINE D3D12CommandAllocator* GetAllocator() const
	{
		return CmdAllocator.Get();
	}

private:
	TSharedPtr<D3D12CommandAllocator> CmdAllocator;
	TSharedPtr<D3D12OnlineDescriptorHeap> OnlineHeap;
	TArray<TSharedRef<PipelineResource>> Resources;
};

/*
* D3D12ResourceBarrierBatcher
*/

class D3D12ResourceBarrierBatcher
{
public:
	inline D3D12ResourceBarrierBatcher()
		: Barriers()
	{
	}

	FORCEINLINE void AddTransitionBarrier(
		D3D12Resource* Resource, 
		D3D12_RESOURCE_STATES BeforeState, 
		D3D12_RESOURCE_STATES AfterState)
	{
		VALIDATE(Resource != nullptr);

		D3D12_RESOURCE_BARRIER Barrier;
		Memory::Memzero(&Barrier, sizeof(D3D12_RESOURCE_BARRIER));

		Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Transition.pResource	= Resource->GetResource();
		Barrier.Transition.StateAfter	= AfterState;
		Barrier.Transition.StateBefore	= BeforeState;
		Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		Barriers.EmplaceBack(Barrier);
	}

	FORCEINLINE void AddUnorderedAccessBarrier(D3D12Resource* Resource)
	{
		VALIDATE(Resource != nullptr);

		D3D12_RESOURCE_BARRIER Barrier;
		Memory::Memzero(&Barrier, sizeof(D3D12_RESOURCE_BARRIER));

		Barrier.Type			= D3D12_RESOURCE_BARRIER_TYPE_UAV;
		Barrier.UAV.pResource	= Resource->GetResource();

		Barriers.EmplaceBack(Barrier);
	}

	FORCEINLINE void FlushBarriers(D3D12CommandList& CmdList)
	{
		if (!Barriers.IsEmpty())
		{
			CmdList.ResourceBarrier(Barriers.Data(), Barriers.Size());
			Barriers.Clear();
		}
	}

	FORCEINLINE const D3D12_RESOURCE_BARRIER* GetBarriers() const
	{
		return Barriers.Data();
	}

	FORCEINLINE UInt32 GetNumBarriers() const
	{
		return Barriers.Size();
	}

private:
	TArray<D3D12_RESOURCE_BARRIER> Barriers;
};

/*
* D3D12CommandContext 
*/

class D3D12CommandContext : public ICommandContext, public D3D12DeviceChild
{
public:
	D3D12CommandContext(class D3D12Device* InDevice, TSharedRef<D3D12CommandQueue>& InCmdQueue, const D3D12DefaultRootSignatures& InDefaultRootSignatures);
	~D3D12CommandContext();

	bool Initialize();

	FORCEINLINE D3D12CommandList& GetCommandList() const
	{
		VALIDATE(CmdList != nullptr);
		return *CmdList;
	}

public:
	virtual void Begin() override final;
	virtual void End()	 override final;

	/*
	* RenderTarget Management
	*/

	virtual void ClearRenderTargetView(
		RenderTargetView* RenderTargetView, 
		const ColorClearValue& ClearColor) override final;
	
	virtual void ClearDepthStencilView(
		DepthStencilView* DepthStencilView, 
		const DepthStencilClearValue& ClearValue) override final;
	
	virtual void ClearUnorderedAccessViewFloat(
		UnorderedAccessView* UnorderedAccessView, 
		const Float ClearColor[4]) override final;

	virtual void BeginRenderPass() override final;
	virtual void EndRenderPass() override final;

	virtual void BindViewport(
		Float Width,
		Float Height,
		Float MinDepth,
		Float MaxDepth,
		Float x,
		Float y) override final;

	virtual void BindScissorRect(
		Float Width,
		Float Height,
		Float x,
		Float y) override final;

	virtual void BindBlendFactor(const ColorClearValue& Color) override final;

	virtual void BindRenderTargets(
		RenderTargetView* const * RenderTargetViews, 
		UInt32 RenderTargetCount, 
		DepthStencilView* DepthStencilView) override final;

	/*
	* Pipeline Management
	*/

	virtual void BindVertexBuffers(
		VertexBuffer* const * VertexBuffers, 
		UInt32 BufferCount, 
		UInt32 BufferSlot) override final;

	virtual void BindPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;
	virtual void BindIndexBuffer(IndexBuffer* IndexBuffer) override final;
	virtual void BindRayTracingScene(RayTracingScene* RayTracingScene) override final;

	virtual void BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState)		override final;
	virtual void BindComputePipelineState(class ComputePipelineState* PipelineState)		override final;
	virtual void BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState)	override final;

	/*
	* Binding Shader Resources
	*/

	// VertexShader
	virtual void VSBindShaderResourceViews(
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void VSBindUnorderedAccessViews(
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void VSBindConstantBuffers(
		ConstantBuffer* const* ConstantBuffers,
		UInt32 ConstantBufferCount,
		UInt32 StartSlot) override final;

	// HullShader
	virtual void HSBindShaderResourceViews(
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void HSBindUnorderedAccessViews(
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void HSBindConstantBuffers(
		ConstantBuffer* const* ConstantBuffers,
		UInt32 ConstantBufferCount,
		UInt32 StartSlot) override final;

	// DomainShader
	virtual void DSBindShaderResourceViews(
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void DSBindUnorderedAccessViews(
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void DSBindConstantBuffers(
		ConstantBuffer* const* ConstantBuffers,
		UInt32 ConstantBufferCount,
		UInt32 StartSlot) override final;

	// GeometryShader
	virtual void GSBindShaderResourceViews(
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void GSBindUnorderedAccessViews(
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void GSBindConstantBuffers(
		ConstantBuffer* const* ConstantBuffers,
		UInt32 ConstantBufferCount,
		UInt32 StartSlot) override final;

	// PixelShader
	virtual void PSBindShaderResourceViews(
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void PSBindUnorderedAccessViews(
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void PSBindConstantBuffers(
		ConstantBuffer* const* ConstantBuffers,
		UInt32 ConstantBufferCount,
		UInt32 StartSlot) override final;

	// ComputeShader
	virtual void CSBindShaderResourceViews(
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void CSBindUnorderedAccessViews(
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void CSBindConstantBuffers(
		ConstantBuffer* const* ConstantBuffers,
		UInt32 ConstantBufferCount,
		UInt32 StartSlot) override final;

	/*
	* Resource Management
	*/

	virtual void UpdateBuffer(
		Buffer* Destination, 
		UInt64 OffsetInBytes, 
		UInt64 SizeInBytes, 
		const Void* SourceData) override final;

	virtual void UpdateTexture2D(
		Texture2D* Destination,
		UInt32 Width,
		UInt32 Height,
		UInt32 MipLevel,
		const Void* SourceData) override final;

	virtual void ResolveTexture(Texture* Destination, Texture* Source) override final;
	
	virtual void CopyBuffer(
		Buffer* Destination, 
		Buffer* Source, 
		const CopyBufferInfo& CopyInfo) override final;

	virtual void CopyTexture(Texture* Destination, Texture* Source) override final;

	virtual void CopyTextureRegion(
		Texture* Destination, 
		Texture* Source, 
		const CopyTextureInfo& CopyTextureInfo) override final;

	virtual void DestroyResource(class PipelineResource* Resource) override final;

	virtual void BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry) 	override final;
	virtual void BuildRayTracingScene(RayTracingScene* RayTracingScene) 			override final;

	virtual void GenerateMips(Texture* Texture) override final;

	/*
	* Resource Barriers
	*/

	virtual void TransitionTexture(
		Texture* Texture,
		EResourceState BeforeState,
		EResourceState AfterState) override final;

	virtual void TransitionBuffer(
		Buffer* Buffer,
		EResourceState BeforeState,
		EResourceState AfterState) override final;

	virtual void UnorderedAccessTextureBarrier(Texture* Texture) override final;

	/*
	* Draw
	*/

	virtual void Draw(
		UInt32 VertexCount, 
		UInt32 StartVertexLocation) override final;
	
	virtual void DrawIndexed(
		UInt32 IndexCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation) override final;
	
	virtual void DrawInstanced(
		UInt32 VertexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartVertexLocation, 
		UInt32 StartInstanceLocation) override final;
	
	virtual void DrawIndexedInstanced(
		UInt32 IndexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation, 
		UInt32 StartInstanceLocation) override final;

	/*
	* Dispatch
	*/

	virtual void Dispatch(
		UInt32 WorkGroupsX, 
		UInt32 WorkGroupsY, 
		UInt32 WorkGroupsZ) override final;

	virtual void DispatchRays(
		UInt32 Width, 
		UInt32 Height, 
		UInt32 Depth) override final;

	/*
	* State
	*/

	virtual void ClearState()	override final;
	virtual void Flush()		override final;

private:
	TSharedRef<D3D12CommandQueue> CmdQueue;
	TSharedRef<D3D12CommandList> CmdList;
	TSharedRef<class D3D12Fence> Fence;
	UInt64 FenceValue = 0;

	TArray<D3D12CommandBatch> CmdBatches;
	UInt32 NextCmdAllocator = 0;
	D3D12CommandBatch* CmdBatch = nullptr;

	D3D12VertexBufferState VertexBufferState;
	D3D12RenderTargetState RenderTargetState;
	D3D12ShaderDescriptorState ShaderDescriptorState;
	D3D12ResourceBarrierBatcher BarrierBatcher;
	D3D12DefaultRootSignatures DefaultRootSignatures;
	D3D12GenerateMipsHelper GenerateMipsHelper;

	Bool IsReady = false;
};