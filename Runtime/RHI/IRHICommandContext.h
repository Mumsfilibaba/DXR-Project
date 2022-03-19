#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHIViewport.h"

#include "Core/Containers/ArrayView.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext : public CRefCounted
{
public:

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) = 0;
    virtual void EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) = 0;

    virtual void ClearRenderTargetTexture(CRHITexture2D* Texture, const float ClearColor[4]) = 0;
    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const float ClearColor[4]) = 0;

    virtual void ClearDepthStencilTexture(CRHITexture2D* Texture, const SRHIDepthStencilValue& ClearValue) = 0;
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SRHIDepthStencilValue& ClearValue) = 0;
    
    virtual void ClearUnorderedAccessTextureFloat(CRHITexture2D* Texture, const float ClearColor[4]) = 0;
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const float ClearColor[4]) = 0;
    virtual void ClearUnorderedAccessTextureUint(CRHITexture2D* Texture, const uint32 ClearColor[4]) = 0;
    virtual void ClearUnorderedAccessViewUint(CRHIUnorderedAccessView* UnorderedAccessView, const uint32 ClearColor[4]) = 0;

    virtual void SetShadingRate(ERHIShadingRate ShadingRate) = 0;
    virtual void SetShadingRateTexture(CRHITexture2D* ShadingImage) = 0;

    virtual void BeginRenderPass(const CRHIRenderPass& RenderPass) = 0;
    virtual void EndRenderPass() = 0;

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) = 0;
    virtual void SetScissorRect(float Width, float Height, float x, float y) = 0;

    virtual void SetBlendFactor(const SColorF& Color) = 0;

    virtual void SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot) = 0;
    virtual void SetIndexBuffer(CRHIBuffer* IndexBuffer, ERHIIndexFormat IndexFormat) = 0;

    virtual void SetPrimitiveTopology(ERHIPrimitiveTopology PrimitveTopologyType) = 0;

    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) = 0;
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) = 0;

    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) = 0;

    virtual void SetShaderResourceTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex) = 0;
    virtual void SetShaderResourceTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex) = 0;
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) = 0;
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 StartParameterIndex) = 0;

    virtual void SetUnorderedAccessTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex) = 0;
    virtual void SetUnorderedAccessTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex) = 0;
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) = 0;
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 StartParameterIndex) = 0;

    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 StartParameterIndex) = 0;

    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) = 0;
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) = 0;

    virtual void UpdateBuffer(CRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    virtual void UpdateConstantBuffer(CRHIConstantBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    virtual void UpdateTexture2D(CRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) = 0;

    virtual void ResolveTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    
    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo) = 0;
    virtual void CopyConstantBuffer(CRHIConstantBuffer* Dst, CRHIConstantBuffer* Src, const SRHICopyBufferInfo& CopyInfo) = 0;
    virtual void CopyTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    virtual void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SRHICopyTextureInfo& CopyTextureInfo) = 0;

    virtual void DestroyResource(class CRHIObject* Resource) = 0;
    virtual void DiscardContents(class CRHIResource* Resource) = 0;

    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate) = 0;
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) = 0;

    /* Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRHIRayTracingShaderResources* GlobalResource,
        const SRHIRayTracingShaderResources* RayGenLocalResources,
        const SRHIRayTracingShaderResources* MissLocalResources,
        const SRHIRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) = 0;

    virtual void GenerateMips(CRHITexture* Texture) = 0;

    virtual void TransitionTexture(CRHITexture* Texture, ERHIResourceState BeforeState, ERHIResourceState AfterState) = 0;
    virtual void TransitionBuffer(CRHIBuffer* Buffer, ERHIResourceState BeforeState, ERHIResourceState AfterState) = 0;
    virtual void TransitionBuffer(CRHIBuffer* Buffer, ERHIResourceState BeforeState, ERHIResourceState AfterState) = 0;

    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) = 0;
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) = 0;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) = 0;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) = 0;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) = 0;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) = 0;

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    virtual void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) = 0;

    virtual void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync) = 0;

    virtual void ClearState() = 0;

    virtual void Flush() = 0;

    virtual void InsertMarker(const String& Message) = 0;

    virtual void BeginExternalCapture() = 0;
    virtual void EndExternalCapture() = 0;
};
