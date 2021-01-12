#pragma once
#include "RenderingCore/ICommandContext.h"

#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"
#include "D3D12SamplerState.h"

class D3D12CommandQueue;
class D3D12CommandAllocator;

/*
* D3D12VertexBufferState
*/

class D3D12VertexBufferState
{
public:
	D3D12VertexBufferState()
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
	D3D12RenderTargetState()
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
			RenderTargetViewHandles[Slot] = { 0 };
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
			DepthStencilViewHandle = { 0 };
		}
	}

	FORCEINLINE void Reset()
	{
		RenderTargetViewHandles.Clear();
		DepthStencilViewHandle = { 0 };
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
* D3D12ShaderDescriptorTableState
*/

class D3D12ShaderDescriptorTableState
{
	struct DescriptorTable
	{
		D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandleStart_GPU = { 0 };
		D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandleStart_CPU = { 0 };

		FORCEINLINE void SetStart(
			D3D12_GPU_DESCRIPTOR_HANDLE InOnlineHandleStart_GPU,
			D3D12_CPU_DESCRIPTOR_HANDLE InOnlineHandleStart_CPU)
		{
			OnlineHandleStart_GPU = InOnlineHandleStart_GPU;
			OnlineHandleStart_CPU = InOnlineHandleStart_CPU;
		}

		FORCEINLINE void Reset()
		{
			OnlineHandleStart_GPU = { 0 };
			OnlineHandleStart_CPU = { 0 };
		}
	};

public:
	D3D12ShaderDescriptorTableState();
	
	Bool CreateResources(D3D12Device& Device);

	void BindConstantBuffer(D3D12ConstantBufferView* ConstantBufferView, UInt32 Slot);
	void BindShaderResourceView(D3D12ShaderResourceView* ShaderResourceView, UInt32 Slot);
	void BindUnorderedAccessView(D3D12UnorderedAccessView* UnorderedAccessView, UInt32 Slot);
	void BindSamplerState(D3D12SamplerState* SamplerState, UInt32 Slot);

	void CommitGraphicsDescriptorTables(
		D3D12Device& Device,
		D3D12OnlineDescriptorHeap& ResourceDescriptorHeap,
		D3D12OnlineDescriptorHeap& SamplerDescriptorHeap,
		D3D12CommandList& CmdList);

	void CommitComputeDescriptorTables(
		D3D12Device& Device,
		D3D12OnlineDescriptorHeap& ResourceDescriptorHeap,
		D3D12OnlineDescriptorHeap& SamplerDescriptorHeap,
		D3D12CommandList& CmdList);

	FORCEINLINE void Reset()
	{
		CBVDescriptorTable.Reset();
		SRVDescriptorTable.Reset();
		UAVDescriptorTable.Reset();
		SamplerDescriptorTable.Reset();

		CBVOfflineHandles.Fill(DefaultCBVOfflineHandle);
		SRVOfflineHandles.Fill(DefaultSRVOfflineHandle);
		UAVOfflineHandles.Fill(DefaultUAVOfflineHandle);
		SamplerOfflineHandles.Fill(DefaultSamplerOfflineHandle);

		IsResourcesDirty	= true;
		IsSamplersDirty		= true;
	}

private:
	void InternalAllocateAndCopyDescriptorHandles(
		D3D12Device& Device,
		D3D12OnlineDescriptorHeap& ResourceDescriptorHeap,
		D3D12OnlineDescriptorHeap& SamplerDescriptorHeap);

	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> CBVOfflineHandles;
	DescriptorTable CBVDescriptorTable;
	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> SRVOfflineHandles;
	DescriptorTable SRVDescriptorTable;
	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> UAVOfflineHandles;
	DescriptorTable UAVDescriptorTable;
	TArray<D3D12_CPU_DESCRIPTOR_HANDLE> SamplerOfflineHandles;
	DescriptorTable SamplerDescriptorTable;

	D3D12_CPU_DESCRIPTOR_HANDLE DefaultCBVOfflineHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE DefaultSRVOfflineHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE DefaultUAVOfflineHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE DefaultSamplerOfflineHandle;

	TSharedRef<D3D12DescriptorHeap> DefaultResourceHeap;
	TSharedRef<D3D12DescriptorHeap> DefaultSamplerHeap;
	TStaticArray<ID3D12DescriptorHeap*, 2> DescriptorHeaps;

	TArray<UInt32> SrcRangeSizes;
	Bool IsResourcesDirty	= false;
	Bool IsSamplersDirty	= false;
};

/*
* D3D12GPUResourceUploader
*/

class D3D12GPUResourceUploader
{
public:
	D3D12GPUResourceUploader()
		: MappedMemory(nullptr)
		, SizeInBytes(0)
		, OffsetInBytes(0)
		, Resource(nullptr)
		, GarbageResources()
	{
	}

	Bool Reserve(D3D12Device& Device, UInt32 InSizeInBytes);
	void Reset();

	Byte* LinearAllocate(D3D12Device& Device, UInt32 SizeInBytes);

	FORCEINLINE ID3D12Resource* GetGpuResource() const
	{
		return Resource.Get();
	}

	FORCEINLINE UInt32 GetOffsetInBytesFromPtr(Byte* Ptr) const
	{
		return UInt32(Ptr - MappedMemory);
	}

	FORCEINLINE UInt32 GetSizeInBytes() const
	{
		return SizeInBytes;
	}

private:
	Byte* MappedMemory		= nullptr;
	UInt32 SizeInBytes		= 0;
	UInt32 OffsetInBytes	= 0;
	TComPtr<ID3D12Resource> Resource;
	TArray<TComPtr<ID3D12Resource>> GarbageResources;
};

/*
* D3D12CommandBatch
*/

class D3D12CommandBatch
{
public:
	D3D12CommandBatch(
		TSharedRef<D3D12CommandAllocator>& InCmdAllocator, 
		TSharedRef<D3D12OnlineDescriptorHeap>& InOnlineResourceDescriptorHeap,
		TSharedRef<D3D12OnlineDescriptorHeap>& InOnlineSamplerDescriptorHeap,
		D3D12GPUResourceUploader InGPUResourceUploader)
		: CmdAllocator(InCmdAllocator)
		, OnlineResourceDescriptorHeap(InOnlineResourceDescriptorHeap)
		, OnlineSamplerDescriptorHeap(InOnlineSamplerDescriptorHeap)
		, GpuResourceUploader(InGPUResourceUploader)
		, Resources()
	{
	}

	FORCEINLINE Bool Reset()
	{
		if (CmdAllocator->Reset())
		{
			Resources.Clear();
			NativeResources.Clear();

			GpuResourceUploader.Reset();

			OnlineResourceDescriptorHeap->Reset();
			OnlineSamplerDescriptorHeap->Reset();

			return true;
		}
		else
		{
			return false;
		}
	}

	FORCEINLINE void EnqueueResourceDestruction(PipelineResource* InResource)
	{
		Resources.EmplaceBack(MakeSharedRef<PipelineResource>(InResource));
	}

	FORCEINLINE void EnqueueResourceDestruction(const TComPtr<ID3D12Resource>& Resource)
	{
		NativeResources.EmplaceBack(Resource);
	}

	FORCEINLINE D3D12GPUResourceUploader& GetGpuResourceUploader()
	{
		return GpuResourceUploader;
	}

	FORCEINLINE D3D12CommandAllocator* GetCommandAllocator() const
	{
		return CmdAllocator.Get();
	}

	FORCEINLINE D3D12OnlineDescriptorHeap* GetOnlineResourceDescriptorHeap() const
	{
		return OnlineResourceDescriptorHeap.Get();
	}

	FORCEINLINE D3D12OnlineDescriptorHeap* GetOnlineSamplerDescriptorHeap() const
	{
		return OnlineSamplerDescriptorHeap.Get();
	}

private:
	TSharedRef<D3D12CommandAllocator>		CmdAllocator;
	TSharedRef<D3D12OnlineDescriptorHeap>	OnlineResourceDescriptorHeap;
	TSharedRef<D3D12OnlineDescriptorHeap>	OnlineSamplerDescriptorHeap;
	
	D3D12GPUResourceUploader				GpuResourceUploader;
	TArray<TSharedRef<PipelineResource>>	Resources;
	TArray<TComPtr<ID3D12Resource>>			NativeResources;
};

/*
* D3D12ResourceBarrierBatcher
*/

class D3D12ResourceBarrierBatcher
{
public:
	D3D12ResourceBarrierBatcher()
		: Barriers()
	{
	}

	void AddTransitionBarrier(
		ID3D12Resource* Resource,
		D3D12_RESOURCE_STATES BeforeState,
		D3D12_RESOURCE_STATES AfterState);

	void AddTransitionBarrier(
		D3D12Resource* Resource,
		D3D12_RESOURCE_STATES BeforeState,
		D3D12_RESOURCE_STATES AfterState);

	FORCEINLINE void AddUnorderedAccessBarrier(D3D12Resource* Resource)
	{
		VALIDATE(Resource != nullptr);

		D3D12_RESOURCE_BARRIER Barrier;
		Memory::Memzero(&Barrier);

		Barrier.Type			= D3D12_RESOURCE_BARRIER_TYPE_UAV;
		Barrier.UAV.pResource	= Resource->GetNativeResource();

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
	D3D12CommandContext(
		D3D12Device* InDevice, 
		TSharedRef<D3D12CommandQueue>& InCmdQueue, 
		const D3D12DefaultRootSignatures& InDefaultRootSignatures);
	
	~D3D12CommandContext();

	Bool Init();

	FORCEINLINE D3D12CommandQueue& GetQueue() const
	{
		VALIDATE(CmdQueue != nullptr);
		return *CmdQueue;
	}

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

	virtual void Bind32BitShaderConstants(
		EShaderStage ShaderStage,
		const Void* Shader32BitConstants,
		UInt32 Num32BitConstants) override final;

	virtual void BindShaderResourceViews(
		EShaderStage ShaderStage,
		ShaderResourceView* const* ShaderResourceViews,
		UInt32 ShaderResourceViewCount,
		UInt32 StartSlot) override final;

	virtual void BindSamplerStates(
		EShaderStage ShaderStage,
		SamplerState* const* SamplerStates,
		UInt32 SamplerStateCount,
		UInt32 StartSlot) override final;

	virtual void BindUnorderedAccessViews(
		EShaderStage ShaderStage,
		UnorderedAccessView* const* UnorderedAccessViews,
		UInt32 UnorderedAccessViewCount,
		UInt32 StartSlot) override final;

	virtual void BindConstantBuffers(
		EShaderStage ShaderStage,
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

	/*
	* Other
	*/

	virtual void InsertMarker(const std::string& Message) override final;

private:
	TSharedRef<D3D12CommandQueue>	CmdQueue;
	TSharedRef<D3D12CommandList>	CmdList;
	TSharedRef<class D3D12Fence>	Fence;
	UInt64 FenceValue = 0;

	TArray<D3D12CommandBatch> CmdBatches;
	UInt32 NextCmdAllocator = 0;
	D3D12CommandBatch* CmdBatch = nullptr;

	TSharedRef<D3D12ComputePipelineState>	GenerateMipsTex2D_PSO;
	TSharedRef<D3D12ComputePipelineState>	GenerateMipsTexCube_PSO;

	D3D12VertexBufferState			VertexBufferState;
	D3D12RenderTargetState			RenderTargetState;
	D3D12ShaderDescriptorTableState	ShaderDescriptorState;
	D3D12ResourceBarrierBatcher		BarrierBatcher;
	D3D12DefaultRootSignatures		DefaultRootSignatures;

	Bool IsReady = false;
};