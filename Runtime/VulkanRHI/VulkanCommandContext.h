#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "RHI/IRHICommandContext.h"
#include "VulkanRHI/VulkanCommandContextState.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanResourceState.h"

class FVulkanDevice;
class FVulkanBufferRHI;
class FVulkanTextureRHI;
class FVulkanCommandBuffer;
class FVulkanShaderResourceViewRHI;
class FVulkanUnorderedAccessViewRHI;
class FVulkanSamplerStateRHI;

class FVulkanBarrierBatcher
{
    struct FBatch
    {
        FBatch(VkDependencyFlags InDependencyFlags)
            : MemoryBarriers()
            , BufferMemoryBarriers()
            , ImageMemoryBarriers()
            , DependencyFlags(InDependencyFlags)
        {
        }

        TArray<VkMemoryBarrier2KHR>       MemoryBarriers;
        TArray<VkBufferMemoryBarrier2KHR> BufferMemoryBarriers;
        TArray<VkImageMemoryBarrier2KHR>  ImageMemoryBarriers;
        VkDependencyFlags                 DependencyFlags;
    };

public:
    void AddMemoryBarrier(VkDependencyFlags DependencyFlags, const VkMemoryBarrier2KHR& InBarrier);
    void AddBufferMemoryBarrier(VkDependencyFlags DependencyFlags, const VkBufferMemoryBarrier2KHR& InBarrier);
    void AddImageMemoryBarrier(VkDependencyFlags DependencyFlags, const VkImageMemoryBarrier2KHR& InIncomingBarrier);
    void FlushBarriers(FVulkanCommandBuffer& CommandBuffer);

#if VK_EXT_sample_locations
    void SetCustomSampleLocations(const VkSampleLocationsInfoEXT* InSampleLocationsInfo);
#endif

    bool HasPendingBarriers() const
    {
        return !Batches.IsEmpty();
    }

private:
    TArray<FBatch>           Batches;
#if VK_EXT_sample_locations
    VkSampleLocationEXT      SampleLocations[RHI_MAX_SAMPLE_POSITIONS] = { };
    VkSampleLocationsInfoEXT SampleLocationsInfo                       = { };
    bool                     bHasCustomSampleLocations                 = false;
#endif
};

class FVulkanCommandContext : public IRHICommandContext, public FVulkanDeviceChild
{
public:
    FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue& InQueue);
    ~FVulkanCommandContext();

    bool Initialize();
    
    // IRHICommandContext Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame()   override final;

    virtual void StartContext()  override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final;
    virtual void EndQuery(FRHIQuery* Query) override final;
    virtual void QueryTimestamp(FRHIQuery* Query) override final;
    virtual void ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor) override final;
    virtual void ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor) override final;
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) override final;
    virtual void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) override final;
    virtual void EndRenderPass() override final;
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const Vector4& Color) override final;
    virtual void SetStencilRef(uint32 StencilRef) override final;
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) override final;
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
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) override final;
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final;
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) override final;
    virtual void WriteFence(FRHIFence* Fence) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;
    virtual void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* InRayTracingScene, const FRHISceneAccelerationStructureBuildDesc& InBuildDesc) override final;
    virtual void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* InRayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& InBuildDesc) override final;
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
    virtual void SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings) override final;
    virtual void BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) override final;
    virtual void ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) override final;
    virtual void SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState) override final;
    virtual void DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth) override final;
    virtual void DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) override final;
    virtual void BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc) override final;
    virtual void ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations) override final;
    virtual void WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources) override final;
    virtual void CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode) override final;
    virtual void CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes) override final;
    virtual void SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset) override final;
    virtual void DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset) override final;
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final;
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace) override final;
    virtual void PushEvent(const StringView& Name) override final;
    virtual void PopEvent() override final;
    
    virtual void ClearState() override final;
    virtual void Flush()      override final;

    virtual void* GetRHINativeCommandList() override final;

    void TransitionImageLayout(FVulkanTextureRHI* Texture, VkImageLayout AfterLayout);
    void TransitionImageLayout(FVulkanTextureRHI* Texture, VkImageLayout BeforeLayout, VkImageLayout AfterLayout);
    void TransitionImageLayout(FVulkanTextureRHI* Texture, VkImageLayout AfterLayout, uint32 FirstMip, uint32 NumMips, uint32 FirstArraySlice, uint32 NumArraySlices);
    void TransitionImageLayout(FVulkanUnorderedAccessViewRHI* View);
    void TransitionImageLayout(FVulkanShaderResourceViewRHI* View, VkImageLayout Layout);

    void RequireBufferState(class FVulkanBufferRHI* Buffer, ERHIResourceState RequiredState);
    void RequireBufferState(FVulkanUnorderedAccessViewRHI* View, ERHIResourceState RequiredState);

    void TransitionBarrierTexture(const FRHITransitionBarrierDesc& Desc);
    void TransitionBarrierBuffer(const FRHITransitionBarrierDesc& Desc);
    
    void ApplyTrackingModeChange(class FVulkanTextureRHI* Texture, const FRHITransitionBarrierDesc& Desc);

    void AddAccelerationStructureMemoryBarrier();

    void ObtainCommandBuffer();
    void FinishCommandBuffer(bool bFlushPool, bool bResolveQueries = true, FVulkanFence** OutFence = nullptr);
    void SplitCommandBuffer(bool bFlushPool, bool bWaitForQueue);

    bool IsRecording()        const { return ContextState.IsRecording(); }
    bool IsInsideRenderPass() const { return ContextState.IsInsideRenderPass(); }
    
    bool NeedsCommandBuffer() const
    {
        return CommandBuffer == nullptr;
    }

    FVulkanCommands& GetCommands()
    {
        return *Commands;
    }

    FVulkanBarrierBatcher& GetBarrierBatcher()
    {
        return BarrierBatcher;
    }
    
    FVulkanQueue& GetCommandQueue() const
    {
        return Queue;
    }

    FVulkanCommandBuffer& GetCommandBuffer()
    {
        CHECK(CommandBuffer != nullptr);
        return *CommandBuffer;
    }

    FVulkanFence* GetSubmissionFence() const
    {
        return Commands ? Commands->Fence : nullptr;
    }

    FVulkanTransientDescriptorAllocator* GetTransientDescriptorAllocator() const
    {
        return TransientDescriptorAllocator;
    }

    FVulkanCommandContextState& GetContextState()
    {
        return ContextState;
    }

private:
    void ConditionalSplitCommandBuffer();
    void ForceFlushCommandPool();
    
    void CloseEventStack();
    void ReopenEventStack();

    FVulkanBufferState&      RetrievePendingBufferState(class FVulkanBufferRHI* Buffer);
    FVulkanImageLayoutState& RetrievePendingImageState(class FVulkanTextureRHI* Texture);

    FVulkanQueue&                                     Queue;
    FVulkanCommandPool*                               CommandPool;
    FVulkanCommandBuffer*                             CommandBuffer;
    FVulkanCommands*                                  Commands;
    FVulkanQueryAllocator                             TimestampQueryAllocator;
    FVulkanQueryAllocator                             OcclusionQueryAllocator;
    FVulkanQueryAllocator                             PipelineStatsQueryAllocator;
    FVulkanBarrierBatcher                             BarrierBatcher;
    TArray<FVulkanQueryRHI*>                          PendingQueries;
    FVulkanCommandContextState                        ContextState;
    TArray<FVulkanPendingImageBarrier>                PendingImageBarriers;
    TArray<FVulkanPendingBufferBarrier>               PendingBufferBarriers;
    TMap<FVulkanTextureRHI*, FVulkanImageLayoutState> PendingImageStates;
    TMap<FVulkanBufferRHI*, FVulkanBufferState>       PendingBufferStates;
    FVulkanTransientDescriptorAllocator*              TransientDescriptorAllocator;
    int32                                             ActiveQueryCount;
    TArray<String>                                    EventStack;

    // TODO: The whole CommandContext should only be used from one thread at a time
    FCriticalSection CommandContextCS;
};
