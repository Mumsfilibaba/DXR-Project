#pragma once
#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHICommandContext

class CNullRHICommandContext : public IRHICommandContext
{
private:

    friend class CNullRHIInstance;

    CNullRHICommandContext() = default;
    ~CNullRHICommandContext() = default;

public:

    static FORCEINLINE CNullRHICommandContext* Make() { return dbg_new CNullRHICommandContext(); }

    virtual void Begin() override final { }
    virtual void End()   override final { }

    virtual void BeginTimeStamp(CRHITimestampQuery* Profiler, uint32 Index) override final { }
    virtual void EndTimeStamp(CRHITimestampQuery* Profiler, uint32 Index)   override final { }

    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const TStaticArray<float, 4>& ClearColor) override final { }
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) override final { }
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor) override final { }

    virtual void SetShadingRate(ERHIShadingRate ShadingRate)      override final { }
    virtual void SetShadingRateImage(CRHITexture2D* ShadingImage) override final { }

    virtual void BeginRenderPass() override final { }
    virtual void EndRenderPass()   override final { }

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final { }
    virtual void SetScissorRect(float Width, float Height, float x, float y) override final { }

    virtual void SetBlendFactor(const TStaticArray<float, 4>& Color) override final { }

    virtual void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView) override final { }

    virtual void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot) override final { }
    virtual void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer) override final { }

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final { }

    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) override final { }
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState)   override final { }

    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final { }

    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final { }
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex) override final { }

    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final { }
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) override final { }

    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) override final { }
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) override final { }

    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) override final { }
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) override final { }

    virtual void UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) override final { }
    virtual void UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) override final { }

    virtual void ResolveTexture(CRHITexture* Destination, CRHITexture* Source) override final { }

    virtual void CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SRHICopyBufferInfo& CopyInfo) override final { }
    virtual void CopyTexture(CRHITexture* Destination, CRHITexture* Source) override final { }
    virtual void CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SRHICopyTextureInfo& CopyTextureInfo) override final { }

    virtual void DestroyResource(class CRHIObject* Resource) override final { }
    virtual void DiscardContents(class CRHIResource* Resource) override final { }

    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate) override final { }
    virtual void BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) override final { }

    virtual void SetRayTracingBindings( CRHIRayTracingScene* RayTracingScene
                                      , CRHIRayTracingPipelineState* PipelineState
                                      , const SRayTracingShaderResources* GlobalResource
                                      , const SRayTracingShaderResources* RayGenLocalResources
                                      , const SRayTracingShaderResources* MissLocalResources
                                      , const SRayTracingShaderResources* HitGroupResources
                                      , uint32 NumHitGroupResources) override final { }

    virtual void GenerateMips(CRHITexture* Texture) override final { }

    virtual void TransitionTexture(CRHITexture* Texture, ERHIResourceState BeforeState, ERHIResourceState AfterState) override final { }
    virtual void TransitionBuffer(CRHIBuffer* Buffer, ERHIResourceState BeforeState, ERHIResourceState AfterState) override final { }

    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) override final { }
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) override final { }

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final { }
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final { }
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final { }
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final { }

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final { }

    virtual void DispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final { }

    virtual void ClearState() override final { }

    virtual void Flush() override final { }

    virtual void InsertMarker(const String& Message) override final { }

    virtual void BeginExternalCapture() override final { }
    virtual void EndExternalCapture() override final { }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif