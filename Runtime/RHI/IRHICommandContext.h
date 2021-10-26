#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"

#include "Core/Containers/ArrayView.h"

class IRHICommandContext : public CRefCounted
{
public:

    /* Start recording commands with this command context */
    virtual void Begin() = 0;

    /* Stop recording commands with this command context */
    virtual void End() = 0;

    virtual void BeginTimeStamp( CGPUProfiler* Profiler, uint32 Index ) = 0;
    virtual void EndTimeStamp( CGPUProfiler* Profiler, uint32 Index ) = 0;

    virtual void ClearRenderTargetView( CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor ) = 0;
    virtual void ClearDepthStencilView( CRHIDepthStencilView* DepthStencilView, const SDepthStencilF& ClearValue ) = 0;
    virtual void ClearUnorderedAccessViewFloat( CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor ) = 0;

    virtual void SetShadingRate( EShadingRate ShadingRate ) = 0;
    virtual void SetShadingRateImage( CRHITexture2D* ShadingImage ) = 0;

    virtual void BeginRenderPass() = 0;
    virtual void EndRenderPass() = 0;

    virtual void SetViewport( float Width, float Height, float MinDepth, float MaxDepth, float x, float y ) = 0;
    virtual void SetScissorRect( float Width, float Height, float x, float y ) = 0;

    virtual void SetBlendFactor( const SColorF& Color ) = 0;

    virtual void SetRenderTargets( CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView ) = 0;

    virtual void SetVertexBuffers( CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot ) = 0;
    virtual void SetIndexBuffer( CRHIIndexBuffer* IndexBuffer ) = 0;

    virtual void SetPrimitiveTopology( EPrimitiveTopology PrimitveTopologyType ) = 0;

    virtual void SetGraphicsPipelineState( class CRHIGraphicsPipelineState* PipelineState ) = 0;
    virtual void SetComputePipelineState( class CRHIComputePipelineState* PipelineState ) = 0;

    virtual void Set32BitShaderConstants( CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants ) = 0;

    virtual void SetShaderResourceView( CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex ) = 0;
    virtual void SetShaderResourceViews( CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex ) = 0;

    virtual void SetUnorderedAccessView( CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex ) = 0;
    virtual void SetUnorderedAccessViews( CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex ) = 0;

    virtual void SetConstantBuffer( CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex ) = 0;
    virtual void SetConstantBuffers( CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex ) = 0;

    virtual void SetSamplerState( CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex ) = 0;
    virtual void SetSamplerStates( CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex ) = 0;

    virtual void UpdateBuffer( CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData ) = 0;
    virtual void UpdateTexture2D( CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData ) = 0;

    virtual void ResolveTexture( CRHITexture* Destination, CRHITexture* Source ) = 0;

    virtual void CopyBuffer( CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo ) = 0;
    virtual void CopyTexture( CRHITexture* Destination, CRHITexture* Source ) = 0;
    virtual void CopyTextureRegion( CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyTextureInfo ) = 0;

    virtual void DiscardResource( class CRHIResource* Resource ) = 0;

    virtual void BuildRayTracingGeometry( CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool Update ) = 0;
    virtual void BuildRayTracingScene( CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update ) = 0;

    virtual void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources ) = 0;

    virtual void GenerateMips( CRHITexture* Texture ) = 0;

    virtual void TransitionTexture( CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState ) = 0;
    virtual void TransitionBuffer( CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState ) = 0;

    virtual void UnorderedAccessTextureBarrier( CRHITexture* Texture ) = 0;
    virtual void UnorderedAccessBufferBarrier( CRHIBuffer* Buffer ) = 0;

    virtual void Draw( uint32 VertexCount, uint32 StartVertexLocation ) = 0;
    virtual void DrawIndexed( uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation ) = 0;
    virtual void DrawInstanced( uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation ) = 0;
    virtual void DrawIndexedInstanced( uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation ) = 0;

    virtual void Dispatch( uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ ) = 0;

    virtual void DispatchRays( CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth ) = 0;

    virtual void ClearState() = 0;
    virtual void Flush() = 0;

    virtual void InsertMarker( const CString& Message ) = 0;

    virtual void BeginExternalCapture() = 0;
    virtual void EndExternalCapture() = 0;
};