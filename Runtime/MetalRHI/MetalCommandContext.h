#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/IRHICommandContext.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalSamplerState.h"

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

    virtual void BeginFrame() override final { }
    virtual void EndFrame() override final { }

    virtual void StartContext() override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final { }
    virtual void EndQuery(FRHIQuery* Query) override final { }
    virtual void QueryTimestamp(FRHIQuery* Query) override final;
    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) override final;
    virtual void BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) override final;
    virtual void EndRenderPass() override final;
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const FVector4& Color) override final;
    virtual void SetStencilRef(uint32 StencilRef) override final;
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) override final;
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets) override final;
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
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
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) override final;
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final;
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) override final;
    virtual void WriteFence(FRHIGpuFence* Fence) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;
    virtual void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) override final;
    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) override final;
    virtual void SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) override final;
    virtual void TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final;
    virtual void TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState) override final;
    virtual void RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState) override final;
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

    virtual void ClearState() override final;

    virtual void Flush() override final;

    virtual void PushEvent(const FStringView& Name) override final;
    virtual void PopEvent() override final;

    virtual void* GetNativeCommandList() override final
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
