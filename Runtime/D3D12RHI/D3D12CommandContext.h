#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "RHI/IRHICommandContext.h"
#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12Queue.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12CommandContextState.h"
#include "D3D12RHI/D3D12ResourceState.h"

class FD3D12ShaderBindingTable;

class FD3D12BarrierBatcher
{
public:
    void AddTransitionBarrier(FD3D12Resource* InResource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
    void AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
    void AddUnorderedAccessBarrier(FD3D12Resource* InResource);
    void AddUnorderedAccessBarrier(ID3D12Resource* Resource);
    void AddAliasingBarrier(FD3D12Resource* InResourceAfter, ID3D12Resource* ResourceBefore = nullptr);
    void AddAliasingBarrier(ID3D12Resource* ResourceAfter, ID3D12Resource* ResourceBefore = nullptr);
    void FlushBarriers(FD3D12CommandList& CommandList);

    bool HasPendingBarriers() const 
    {
        return !Barriers.IsEmpty();
    }

private:
    TArray<D3D12_RESOURCE_BARRIER> Barriers;
};

class FD3D12CommandContext : public IRHICommandContext, public FD3D12DeviceChild
{
public:
    FD3D12CommandContext(FD3D12Device* InDevice, FD3D12Queue& InQueue);
    ~FD3D12CommandContext();

    bool Initialize();

    // IRHICommandContext Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual void StartContext() override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final;
    virtual void EndQuery(FRHIQuery* Query) override final;
    virtual void QueryTimestamp(FRHIQuery* Query) override final;
    virtual void ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor) override final;
    virtual void ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor) override final;
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) override final;
    virtual void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) override final;
    virtual void EndRenderPass() override final { }
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const Vector4& Color) override final;
    virtual void SetStencilRef(uint32 StencilRef) override final;
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) override final;
    virtual void SetDepthBounds(float MinDepth, float MaxDepth) override final;
    virtual void SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc) override final;
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets) override final;
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
    virtual void SetMeshletPipelineState(class FRHIMeshletPipelineState* PipelineState) override final;
    virtual void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) override final;
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex) override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex) override final;
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex) override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex) override final;
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex) override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex) override final;
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex) override final;
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex) override final;
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;
    virtual void UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch) override final;
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void TranscodeSamplerFeedback(FRHITexture* Dst, uint32 DstSubresource, FRHITexture* Src, uint32 SrcSubresource, ESamplerFeedbackTranscodeMode Mode) override final;
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) override final;
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final;
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) override final;
    virtual void WriteFence(FRHIFence* Fence) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;
    virtual void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc) override final;
    virtual void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) override final;
    virtual void WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources) override final;
    virtual void CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode) override final;
    virtual void CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes) override final;
    virtual void SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset) override final;
    virtual void DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset) override final;
    virtual void SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings) override final;
    virtual void BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) override final;
    virtual void ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) override final;
    virtual void SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState) override final;
    virtual void DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth) override final;
    virtual void DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) override final;
    virtual void BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc) override final;
    virtual void ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations) override final;
    virtual void TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs) override final;
    virtual void UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs) override final;
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;
    virtual void DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override final;
    virtual void DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final;
    virtual void DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) override final;
    virtual void DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final;
    virtual void DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) override final;
    virtual void DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) override final;
    virtual void DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final;
    virtual void DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) override final;
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final;
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace) override final;
    virtual void SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata) override final;
    virtual void PushEvent(const StringView& Name) override final;
    virtual void PopEvent() override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void* GetRHINativeCommandList() override final;

    void ObtainCommandList();
    void FinishCommandList(bool bFlushAllocator, bool bResolveQueries = true, FD3D12FenceSyncPoint* OutSyncPoint = nullptr);
    void SplitCommandList(bool bFlushAllocator, bool bWaitForQueue);
    void SplitCommandListAndResetState(bool bFlushAllocator, bool bWaitForQueue);
    void SplitCommandListForDescriptorHeapRollover();
    bool HasAvailableDescriptorBlock() const;

    void RetireTransientObjects();
    void DeferDescriptorBlockRecycle(FD3D12OnlineDescriptorHeap& Heap, FD3D12OnlineDescriptorBlock* Block);
    
    void UpdateBuffer(FD3D12Resource* Resource, const FBufferRegion& BufferRegion, const void* SourceData);
    
    void TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState);
    void TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);
    void TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState, uint32 FirstMip, uint32 NumMips, uint32 FirstArraySlice, uint32 NumArraySlices);
    
    void TransitionResourceState(FD3D12UnorderedAccessViewRHI* View);
    void TransitionResourceState(FD3D12ShaderResourceViewRHI* View, D3D12_RESOURCE_STATES State);
    void TransitionResourceState(FD3D12RenderTargetViewRHI* View);
    void TransitionResourceState(FD3D12DepthStencilViewRHI* View, D3D12_RESOURCE_STATES State);

    void TransitionTrackedResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState);
    void TransitionTrackedResourceState(FD3D12BufferRHI* Buffer, D3D12_RESOURCE_STATES AfterState);
    void TransitionTrackedResourceState(FD3D12TextureRHI* Texture, D3D12_RESOURCE_STATES AfterState);

    void AliasingBarrier(FD3D12Resource* ResourceAfter, ID3D12Resource* ResourceBefore = nullptr);

#if D3D12_VALIDATE_CONTEXT_THREAD_OWNERSHIP
    void AcquireOwnership();
    void ReleaseOwnership();
    void VerifyOwnerThread()     const;
    void VerifyExclusiveAccess() const;
#else
    FORCEINLINE void AcquireOwnership()            { }
    FORCEINLINE void ReleaseOwnership()            { }
    FORCEINLINE void VerifyOwnerThread()     const { }
    FORCEINLINE void VerifyExclusiveAccess() const { }
#endif

    FORCEINLINE FD3D12CommandList& GetCommandList() 
    {
        CHECK(CommandList != nullptr);
        return *CommandList; 
    }

    FORCEINLINE FD3D12Commands& GetCommands()
    {
        CHECK(Commands != nullptr);
        return *Commands;
    }

    FORCEINLINE FD3D12BarrierBatcher& GetBarrierBatcher()
    {
        return BarrierBatcher;
    }

    FORCEINLINE FD3D12Queue& GetQueue() const
    {
        return Queue;
    }

    FORCEINLINE ED3D12CommandQueueType GetQueueType() const
    {
        return Queue.GetQueueType();
    }

    FORCEINLINE bool IsRecording() const
    {
        return bIsRecording;
    }

    FORCEINLINE bool NeedsCommandList() const
    {
        return CommandList == nullptr;
    }

    FORCEINLINE void SetLastUsedFrame(uint64 InFrame)
    {
        LastUsedFrame = InFrame;
    }

    FORCEINLINE uint64 GetLastUsedFrame() const
    {
        return LastUsedFrame;
    }

private:
    void ConditionalSplitCommandList();
    void PrepareShaderBindingTableForDispatch(FD3D12ShaderBindingTable* ShaderBindingTable);
    void CloseEventStack();
    void ReopenEventStack();

    FD3D12ResourceState& RetrievePendingResourceState(FD3D12Resource* Resource);
    void AddPendingBarrier(FD3D12Resource* Resource, D3D12_RESOURCE_STATES DesiredState, uint32 Subresource);

    void TransitionBarrierTexture(const FRHITransitionBarrierDesc& Desc);
    void TransitionBarrierBuffer(const FRHITransitionBarrierDesc& Desc);

    bool EmitTrackedTransition(FD3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex, D3D12_RESOURCE_BARRIER_FLAGS BarrierFlags);
    void ApplyTrackingModeChange(FD3D12TextureRHI* Texture, const FRHITransitionBarrierDesc& Desc);

    void EnsureDefaultState(const FD3D12Resource* Resource) const;
    void EnsureResourceState(const FD3D12Resource* Resource, D3D12_RESOURCE_STATES RequiredState) const;

    NODISCARD D3D12_RESOURCE_STATES GetTrackedResourceState(const FD3D12Resource* Resource) const;

    FD3D12CommandList*                         CommandList;
    FD3D12CommandAllocator*                    CommandAllocator;
    FD3D12Commands*                            Commands;
    FD3D12CommandContextState                  ContextState;
    FD3D12QueryAllocator                       TimingQueryAllocator;
    FD3D12QueryAllocator                       OcclusionQueryAllocator;
    FD3D12QueryAllocator                       PipelineStatsQueryAllocator;
    FD3D12BarrierBatcher                       BarrierBatcher;
    TArray<FD3D12PendingBarrier>               PendingBarriers;
    TMap<FD3D12Resource*, FD3D12ResourceState> PendingResourceStates;
    TArray<FD3D12QueryRHI*>                    PendingQueries;
    TArray<FD3D12DeferredObject>               DeferredObjects;
    FD3D12Queue&                               Queue;
    TArray<String>                             EventStack;
    uint64                                     LastUsedFrame;
    int32                                      ActiveQueryCount;
#if D3D12_VALIDATE_CONTEXT_THREAD_OWNERSHIP
    TAtomicInt<uint32>                         OwnerThreadID;
#endif
    bool                                       bIsRecording : 1;
};

class FD3D12BorrowedCommandContext : FNonCopyable
{
public:
    explicit FD3D12BorrowedCommandContext(FD3D12Queue& InQueue)
        : Queue(InQueue)
        , Context(InQueue.ObtainCommandContext())
    {
    }

    ~FD3D12BorrowedCommandContext()
    {
        Queue.ReleaseCommandContext(Context);
    }

    FORCEINLINE FD3D12CommandContext* Get() const
    {
        return Context;
    }

    FORCEINLINE FD3D12CommandContext& operator*() const
    {
        return *Context;
    }

    FORCEINLINE FD3D12CommandContext* operator->() const
    {
        return Context;
    }

protected:
    FD3D12Queue&          Queue;
    FD3D12CommandContext* Context;
};

class FD3D12ScopedCommandContext : public FD3D12BorrowedCommandContext
{
public:
    explicit FD3D12ScopedCommandContext(FD3D12Queue& InQueue)
        : FD3D12BorrowedCommandContext(InQueue)
    {
        Context->StartContext();
    }

    ~FD3D12ScopedCommandContext()
    {
        Context->FinishContext();
    }
};
