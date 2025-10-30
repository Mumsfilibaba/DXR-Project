#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "RHI/IRHICommandContext.h"
#include "VulkanRHI/VulkanCommandContextState.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanQuery.h"

class FVulkanDevice;
class FVulkanBuffer;
class FVulkanCommandContext;

class FBarrierBatcher
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

        TArray<VkMemoryBarrier2>       MemoryBarriers;
        TArray<VkBufferMemoryBarrier2> BufferMemoryBarriers;
        TArray<VkImageMemoryBarrier2>  ImageMemoryBarriers;
        VkDependencyFlags              DependencyFlags;
    };

public:
    FBarrierBatcher(FVulkanCommandContext& InContext);
    ~FBarrierBatcher() = default;

    void AddMemoryBarrier(VkDependencyFlags DependencyFlags, const VkMemoryBarrier2& InBarrier);
    void AddBufferMemoryBarrier(VkDependencyFlags DependencyFlags, const VkBufferMemoryBarrier2& InBarrier);
    void AddImageMemoryBarrier(VkDependencyFlags DependencyFlags, const VkImageMemoryBarrier2& InBarrier);
    void FlushBarriers();
    
    bool HasPendingBarriers() const
    {
        return !Batches.IsEmpty();
    }

private:
    FVulkanCommandContext& Context;
    TArray<FBatch>         Batches;
};

class FVulkanCommandContext : public IRHICommandContext, public FVulkanDeviceChild
{
public:
    FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue& InQueue);
    ~FVulkanCommandContext();

    // IRHICommandContext Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual void StartContext() override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final;
    virtual void EndQuery(FRHIQuery* Query) override final;
    virtual void QueryTimestamp(FRHIQuery* Query) override final;
    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;
    virtual void BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) override final;
    virtual void EndRenderPass() override final;
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const FVector4& Color) override final;
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
    virtual void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;
    virtual void BuildRayTracingScene(FRHIRayTracingScene* InRayTracingScene, const FRayTracingSceneBuildInfo& InBuildInfo) override final;
    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* InRayTracingGeometry, const FRayTracingGeometryBuildInfo& InBuildInfo) override final;
    virtual void SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) override final;
    virtual void TransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final;
    virtual void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final;
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;
    virtual void DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final;
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height) override final;
    virtual void InsertMarker(const FStringView& Message) override final;
    
    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture() override final;

    virtual void* GetNativeCommandList() override final 
    { 
        return reinterpret_cast<void*>(&CommandBuffer);
    }

    bool Initialize();
    void ObtainCommandBuffer();
    void FinishCommandBuffer(bool bFlushPool);
    void SplitCommandBuffer(bool bFlushPool, bool bWaitForQueue);

    FVulkanQueue& GetCommandQueue() const
    {
        return Queue;
    }

    FVulkanCommandBuffer& GetCommandBuffer()
    {
        CHECK(CommandBuffer != nullptr);
        return *CommandBuffer;
    }

    FBarrierBatcher& GetBarrierBatcher()
    {
        return BarrierBatcher;
    }

    FVulkanCommandPayload& GetCommandPayload()
    {
        return *CommandPayload;
    }

    FVulkanFence* GetSubmissionFence() const
    {
        return CommandPayload ? CommandPayload->Fence : nullptr;
    }

    bool IsRecording() const { return ContextPhase >= ECommandContextPhase::Recording; }
    bool IsInsideRenderPass() const { return ContextPhase == ECommandContextPhase::InsideRenderPass; }
    
    bool NeedsCommandBuffer() const
    {
        return CommandBuffer == nullptr;
    }

private:
    void ForceFlushCommandPool();

    FVulkanQueue&              Queue;
    FVulkanCommandPool*        CommandPool;
    FVulkanCommandBuffer*      CommandBuffer;
    FVulkanCommandPayload*     CommandPayload;
    FVulkanQueryAllocator      TimestampQueryAllocator;
    FVulkanQueryAllocator      OcclusionQueryAllocator;
    FBarrierBatcher            BarrierBatcher;
    ECommandContextPhase       ContextPhase;
    FVulkanCommandContextState ContextState;

    // TODO: The whole CommandContext should only be used from one thread at a time
    FCriticalSection CommandContextCS;
};
