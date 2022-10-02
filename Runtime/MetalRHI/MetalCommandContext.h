#pragma once
#include "MetalPipelineState.h"
#include "MetalBuffer.h"
#include "MetalViews.h"
#include "MetalSamplerState.h"

#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalDeviceContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalCopyCommandContext

class FMetalCopyCommandContext final
{
public:

    FMetalCopyCommandContext()
        : CopyEncoder(nil)
    { }
    
    void StartContext(id<MTLCommandBuffer> CommandBuffer)
    {
        if (!CopyEncoder)
        {
            CopyEncoder = [CommandBuffer blitCommandEncoder];
        }
    }
    
    void FinishContext()
    {
        if (CopyEncoder)
        {
            [CopyEncoder endEncoding];
            NSRelease(CopyEncoder);
        }
    }
    
    void FinishContextUnsafe()
    {
        Check(CopyEncoder != nil);
        
        [CopyEncoder endEncoding];
        NSRelease(CopyEncoder);
    }
    
    id<MTLBlitCommandEncoder> GetMTLCopyEncoder() const { return CopyEncoder; };
    
private:
    id<MTLBlitCommandEncoder> CopyEncoder;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalCommandContext

class FMetalCommandContext final 
    : public FMetalObject
    , public IRHICommandContext
{
private:

    friend class FMetalInterface;

    FMetalCommandContext(FMetalDeviceContext* InDeviceContext);
    ~FMetalCommandContext() = default;

public:
    static FMetalCommandContext* CreateMetalContext(FMetalDeviceContext* InDeviceContext);

    virtual void StartContext()  override final;
    virtual void FinishContext() override final;

    virtual void BeginTimeStamp(FRHITimestampQuery* Profiler, uint32 Index) override final;
    virtual void EndTimeStamp(FRHITimestampQuery* Profiler, uint32 Index)   override final;

    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)         override final;
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)                 override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;

    virtual void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer) override final;
    virtual void EndRenderPass()                                                         override final;

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final;
    virtual void SetScissorRect(float Width, float Height, float x, float y)                              override final;

    virtual void SetBlendFactor(const FVector4& Color) override final;

    virtual void SetVertexBuffers(const TArrayView<FRHIVertexBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)                                                   override final;

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;

    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState)   override final;

    virtual void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;

    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)                             override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;

    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)                             override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;

    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)                             override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIConstantBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;

    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)                             override final;
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;

    virtual void UpdateBuffer(FRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)           override final;
    virtual void UpdateTexture2D(FRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) override final;

    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;

    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo)                  override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src)                                                   override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo) override final;

    virtual void DestroyResource(class IRefCounted* Resource) override final;
    virtual void DiscardContents(class FRHITexture* Texture)   override final;

    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)       override final;
    virtual void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate) override final;

    virtual void SetRayTracingBindings(
        FRHIRayTracingScene* RayTracingScene,
        FRHIRayTracingPipelineState* PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) override final;

    virtual void GenerateMips(FRHITexture* Texture) override final;

    virtual void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)    override final;

    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)    override final;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation)                                                                                                         override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)                                                                         override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)                                 override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;

    virtual void DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;

    virtual void PresentViewport(FRHIViewport* Viewport, bool bVerticalSync) override final;
    
    virtual void ClearState() override final;

    virtual void Flush() override final;

    virtual void InsertMarker(const FStringView& Message) override final;

    // NOTE: Only supported in D3D12RHI for now
    virtual void BeginExternalCapture() override final { }
    virtual void EndExternalCapture()   override final { }

    virtual void* GetRHIBaseCommandList() override final { return reinterpret_cast<void*>(CommandBuffer); }
    
private:
    void PrepareForDraw();
    
    id<MTLCommandBuffer>        CommandBuffer;
    
    // RenderEncoder
    id<MTLRenderCommandEncoder> GraphicsEncoder;
    MTLViewport                 CurrentViewport;
    
    // PipelineState
    TSharedRef<FMetalIndexBuffer>           CurrentIndexBuffer;
    TSharedRef<FMetalGraphicsPipelineState> CurrentGraphicsPipeline;
    MTLPrimitiveType                        CurrentPrimitiveType;

    // VertexBuffer- state
    TStaticArray<id<MTLBuffer>, kRHIMaxVertexBuffers> CurrentVertexBuffers;
    TStaticArray<NSUInteger   , kRHIMaxVertexBuffers> CurrentVertexOffsets;
    NSRange CurrentVertexBufferRange;
    
    // Resources
    enum
    {
        kMaxSRVs            = 16,
        kMaxUAVs            = 16,
        kMaxConstantBuffers = 16,
        kMaxSamplerStates   = 16,
    };
    
    TStaticArray<TSharedRef<FMetalSamplerState>       , kMaxSamplerStates>   CurrentSamplerStates[ShaderVisibility_Count];
    TStaticArray<TSharedRef<FMetalShaderResourceView> , kMaxSRVs>            CurrentSRVs[ShaderVisibility_Count];
    TStaticArray<TSharedRef<FMetalUnorderedAccessView>, kMaxUAVs>            CurrentUAVs[ShaderVisibility_Count];
    TStaticArray<TSharedRef<FMetalConstantBuffer>     , kMaxConstantBuffers> CurrentConstantBuffers[ShaderVisibility_Count];
    
    TStaticArray<id<MTLBuffer> , kMaxBuffers>  CurrentBuffers[ShaderVisibility_Count];
    TStaticArray<id<MTLTexture>, kMaxTextures> CurrentTextures[ShaderVisibility_Count];
    
    // Contexts
    FMetalCopyCommandContext CopyContext;
    
};

#pragma clang diagnostic pop
