#pragma once
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRefCounted.h"
#include "VulkanPipelineState.h"
#include "VulkanDescriptorCache.h"
#include "RHI/IRHICommandContext.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"

class FVulkanDevice;
class FVulkanBuffer;

struct FVulkanCommandContextState : public FVulkanDeviceObject, public FNonCopyAndNonMovable
{
    FVulkanCommandContextState(FVulkanDevice* InDevice);
    ~FVulkanCommandContextState() = default;

    bool Initialize();

    void ApplyGraphics(FVulkanCommandBuffer& CommandBuffer);
    void ApplyCompute(FVulkanCommandBuffer& CommandBuffer);

    void ClearGraphics();
    void ClearCompute();

    void SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 Slot);
    void SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexFormat);

    void ClearAll()
    {
        ClearGraphics();
        ClearCompute();

        // DescriptorCache.Clear();
        // PushConstantsCache.Reset();

        bIsReady            = false;
        bIsCapturing        = false;
        bIsRenderPassActive = false;
        bBindPipelineLayout = true;
    }

    struct
    {
        FVulkanGraphicsPipelineStateRef PipelineState;

        // FD3D12TextureRef            ShadingRateTexture;
        // D3D12_SHADING_RATE          ShadingRate = D3D12_SHADING_RATE_1X1;

        FVulkanIndexBufferCache  IBCache;
        FVulkanVertexBufferCache VBCache;

        VkViewport Viewports[32];
        uint32     NumViewports;

        VkRect2D   ScissorRects[32];
        uint32     NumScissor;

        FVector4 BlendFactor;

        bool bBindRenderTargets     : 1;
        bool bBindBlendFactor       : 1;
        bool bBindPipeline          : 1;
        bool bBindVertexBuffers     : 1;
        bool bBindIndexBuffer       : 1;
        bool bBindScissorRects      : 1;
        bool bBindViewports         : 1;
    } Graphics;

    struct
    {
        FVulkanComputePipelineStateRef PipelineState;
        bool bBindPipeline : 1;
    } Compute;

    FVulkanPushConstantsCache PushConstantsCache;

    bool bIsReady            : 1;
    bool bIsCapturing        : 1;
    bool bIsRenderPassActive : 1;
    bool bBindPipelineLayout : 1;
};


class FVulkanCommandContext : public IRHICommandContext, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue* InCommandQueue);
    ~FVulkanCommandContext();

    virtual void RHIStartContext() override final;
    
    virtual void RHIFinishContext() override final;

    virtual void RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) override final;

    virtual void RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) override final;

    virtual void RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    
    virtual void RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final;
    
    virtual void RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;

    virtual void RHIBeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer) override final;
    
    virtual void RHIEndRenderPass() override final;

    virtual void RHISetViewport(const FRHIViewportRegion& ViewportRegion) override final;

    virtual void RHISetScissorRect(const FRHIScissorRegion& ScissorRegion) override final;

    virtual void RHISetBlendFactor(const FVector4& Color) override final;

    virtual void RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    
    virtual void RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;

    virtual void RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    
    virtual void RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;

    virtual void RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;

    virtual void RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    
    virtual void RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;

    virtual void RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    
    virtual void RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;

    virtual void RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    
    virtual void RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;

    virtual void RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    
    virtual void RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;

    virtual void RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    
    virtual void RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;

    virtual void RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;

    virtual void RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final;
    
    virtual void RHICopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    
    virtual void RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) override final;

    virtual void RHIDestroyResource(class IRefCounted* Resource) override final;

    virtual void RHIDiscardContents(class FRHITexture* Texture) override final;

    virtual void RHIBuildRayTracingGeometry(
        FRHIRayTracingGeometry* RayTracingGeometry,
        FRHIBuffer*             VertexBuffer,
        uint32                  NumVertices,
        FRHIBuffer*             IndexBuffer,
        uint32                  NumIndices,
        EIndexFormat            IndexFormat,
        bool                    bUpdate) override final;
    
    virtual void RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate) override final;

    virtual void RHISetRayTracingBindings(
        FRHIRayTracingScene*              RayTracingScene,
        FRHIRayTracingPipelineState*      PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32                            NumHitGroupResources) override final;

    virtual void RHIGenerateMips(FRHITexture* Texture) override final;

    virtual void RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) override final;

    virtual void RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;

    virtual void RHIUnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    
    virtual void RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final;

    virtual void RHIDraw(uint32 VertexCount, uint32 StartVertexLocation) override final;

    virtual void RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    
    virtual void RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    
    virtual void RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;

    virtual void RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;

    virtual void RHIDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;

    virtual void RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync) override final;

    virtual void RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height) override final;

    virtual void RHIClearState() override final;

    virtual void RHIFlush() override final;

    virtual void RHIInsertMarker(const FStringView& Message) override final;

    virtual void RHIBeginExternalCapture() override final;

    virtual void RHIEndExternalCapture() override final;

    virtual void* RHIGetNativeCommandList() override final 
    { 
        return reinterpret_cast<void*>(&CommandBuffer);
    }

public:
    bool Initialize();

    void ImageLayoutTransitionBarrier(const FVulkanImageTransitionBarrier& TransitionBarrier);

    void ObtainCommandBuffer();
    
    void FlushCommandBuffer();

    FVulkanQueue* GetCommandQueue() const
    {
        return Queue.Get();
    }

    FVulkanCommandBuffer* GetCommandBuffer()
    {
        return &CommandBuffer;
    }

    const FVulkanCommandBuffer* GetCommandBuffer() const
    {
        return &CommandBuffer;
    }

private:
    FVulkanQueueRef            Queue;
    FVulkanCommandBuffer       CommandBuffer;
    FVulkanCommandContextState ContextState;
    
    // TODO: The whole commandcontext should only be used from one thread at a time
    FCriticalSection CommandContextCS;

    bool bRenderPass = false;
    
    // These resources should be destroyed, most likely there is no reference left in the higher level
    TArray<TSharedRef<IRefCounted>>       DiscardList;
    TArray<TSharedRef<FVulkanRefCounted>> DiscardListVk;
};
