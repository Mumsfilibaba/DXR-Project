#pragma once
#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHICommandContext

class FNullRHICommandContext final 
    : public IRHICommandContext
{
private:
    friend class FNullRHICoreInterface;

    FNullRHICommandContext()  = default;
    ~FNullRHICommandContext() = default;

public:
    static FNullRHICommandContext* CreateNullRHIContext() { return dbg_new FNullRHICommandContext(); }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRHICommandContext Interface

    virtual void StartContext()  override final { }
    virtual void FinishContext() override final { }

    virtual void BeginTimeStamp(FRHITimestampQuery* Profiler, uint32 Index) override final { }
    virtual void EndTimeStamp(FRHITimestampQuery* Profiler, uint32 Index)   override final { }

    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor) override final { }
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final { }
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor) override final { }

    virtual void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer) override final { }
    virtual void EndRenderPass() override final { }

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final { }
    virtual void SetScissorRect(float Width, float Height, float x, float y) override final { }

    virtual void SetBlendFactor(const TStaticArray<float, 4>& Color) override final { }

    virtual void SetVertexBuffers(FRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot) override final { }
    virtual void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer) override final { }

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final { }

    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final { }
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState)   override final { }

    virtual void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final { }

    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final { }
    virtual void SetShaderResourceViews(FRHIShader* Shader, FRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex) override final { }

    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final { }
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, FRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) override final { }

    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) override final { }
    virtual void SetConstantBuffers(FRHIShader* Shader, FRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) override final { }

    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final { }
    virtual void SetSamplerStates(FRHIShader* Shader, FRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) override final { }

    virtual void UpdateBuffer(FRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) override final { }
    virtual void UpdateTexture2D(FRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) override final { }

    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final { }

    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo) override final { }
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final { }
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo) override final { }

    virtual void DestroyResource(class IRefCounted* Resource) override final { }

    virtual void DiscardContents(class FRHITexture* Texture) override final { }

    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)       override final { }
    virtual void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate) override final { }

    virtual void SetRayTracingBindings(
        FRHIRayTracingScene* RayTracingScene,
        FRHIRayTracingPipelineState* PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) override final { }

    virtual void GenerateMips(FRHITexture* Texture) override final { }

    virtual void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) override final { }
    virtual void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)    override final { }

    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final { }
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)    override final { }

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final { }
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final { }
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final { }
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final { }

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final { }

    virtual void DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final { }

    virtual void PresentViewport(FRHIViewport* Viewport, bool bVerticalSync) override final { }

    virtual void ClearState() override final { }

    virtual void Flush() override final { }

    virtual void InsertMarker(const FStringView& Message) override final { }

    virtual void BeginExternalCapture() override final { }
    virtual void EndExternalCapture()   override final { }

    virtual void* GetRHIBaseCommandList() override final { return nullptr; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
