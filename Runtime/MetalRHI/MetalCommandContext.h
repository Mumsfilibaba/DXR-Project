#pragma once
#include "MetalPipelineState.h"
#include "MetalBuffer.h"
#include "MetalViews.h"
#include "MetalSamplerState.h"

#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalDeviceContext;

class FMetalCopyCommandContext final
{
public:
    FMetalCopyCommandContext()
        : CopyEncoder(nil)
    {
    }
    
    void StartEncoder(id<MTLCommandBuffer> CommandBuffer)
    {
        if (!CopyEncoder)
        {
            CopyEncoder = [CommandBuffer blitCommandEncoder];
        }
    }
    
    void FinishEncoder()
    {
        if (CopyEncoder)
        {
            [CopyEncoder endEncoding];
            [CopyEncoder release];
        }
    }
    
    void FinishEncoderUnsafe()
    {
        CHECK(CopyEncoder != nil);
        [CopyEncoder endEncoding];
        [CopyEncoder release];
    }
    
    id<MTLBlitCommandEncoder> GetMTLCopyEncoder() const 
    {
        return CopyEncoder;
    };
    
private:
    id<MTLBlitCommandEncoder> CopyEncoder;
};

class FMetalCommandContext final : public FMetalDeviceChild, public IRHICommandContext
{
    friend class FMetalRHI;

    FMetalCommandContext(FMetalDeviceContext* InDeviceContext);
    ~FMetalCommandContext() = default;

public:
    static FMetalCommandContext* CreateMetalContext(FMetalDeviceContext* InDeviceContext);

    virtual void RHIBeginFrame() override final { }
    virtual void RHIEndFrame() override final { }

    virtual void RHIStartContext() override final;
    virtual void RHIFinishContext() override final;

    virtual void RHIBeginQuery(FRHIQuery* Query) override final { }
    virtual void RHIEndQuery(FRHIQuery* Query) override final { }
    virtual void RHIQueryTimestamp(FRHIQuery* Query) override final;
    virtual void RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    virtual void RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;
    virtual void RHIBeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) override final;
    virtual void RHIEndRenderPass() override final;
    virtual void RHISetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void RHISetScissorRect(const FScissorRegion& ScissorRegion) override final;
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
    virtual void RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) override final;
    virtual void RHICopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) override final;
    virtual void RHIDiscardContents(class FRHITexture* Texture) override final;
    virtual void RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) override final;
    virtual void RHIBuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) override final;
    virtual void RHISetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) override final;
    virtual void RHIGenerateMips(FRHITexture* Texture) override final;
    virtual void RHITransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final;
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
        return nullptr;
    }
    
private:
    void PrepareForDraw();
    
    id<MTLCommandBuffer>        CommandBuffer;
    
    // RenderEncoder
    id<MTLRenderCommandEncoder> GraphicsEncoder;
    MTLViewport                 CurrentViewport;
    
    // PipelineState
    FMetalBufferRef                         CurrentIndexBuffer;
    TSharedRef<FMetalGraphicsPipelineState> CurrentGraphicsPipeline;
    MTLPrimitiveType                        CurrentPrimitiveType;

    // VertexBuffer- state
    TStaticArray<id<MTLBuffer>, RHI_MAX_VERTEX_BUFFERS> CurrentVertexBuffers;
    TStaticArray<NSUInteger   , RHI_MAX_VERTEX_BUFFERS> CurrentVertexOffsets;
    NSRange CurrentVertexBufferRange;
    
    // Resources
    enum
    {
        kMaxSRVs            = 16,
        kMaxUAVs            = 16,
        kMaxConstantBuffers = 16,
        kMaxSamplerStates   = 16,
    };
    
    TStaticArray<FMetalSamplerStateRef, kMaxSamplerStates>        CurrentSamplerStates[ShaderVisibility_Count];
    TStaticArray<TSharedRef<FMetalShaderResourceView>, kMaxSRVs>  CurrentSRVs[ShaderVisibility_Count];
    TStaticArray<TSharedRef<FMetalUnorderedAccessView>, kMaxUAVs> CurrentUAVs[ShaderVisibility_Count];
    TStaticArray<FMetalBufferRef, kMaxConstantBuffers>            CurrentConstantBuffers[ShaderVisibility_Count];
    
    TStaticArray<id<MTLBuffer> , kMaxBuffers>  CurrentBuffers[ShaderVisibility_Count];
    TStaticArray<id<MTLTexture>, kMaxTextures> CurrentTextures[ShaderVisibility_Count];
    
    // Contexts
    FMetalCopyCommandContext CopyContext;
    
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
