#pragma once
#include "RenderingCore.h"
#include "Resources.h"
#include "ResourceViews.h"

#include "Core/Containers/ArrayView.h"

class ICommandContext : public RefCountedObject
{
public:
    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginTimeStamp( GPUProfiler* Profiler, uint32 Index ) = 0;
    virtual void EndTimeStamp( GPUProfiler* Profiler, uint32 Index ) = 0;

    virtual void ClearRenderTargetView( RenderTargetView* RenderTargetView, const ColorF& ClearColor ) = 0;
    virtual void ClearDepthStencilView( DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue ) = 0;
    virtual void ClearUnorderedAccessViewFloat( UnorderedAccessView* UnorderedAccessView, const ColorF& ClearColor ) = 0;

    virtual void SetShadingRate( EShadingRate ShadingRate ) = 0;
    virtual void SetShadingRateImage( Texture2D* ShadingImage ) = 0;

    virtual void BeginRenderPass() = 0;
    virtual void EndRenderPass() = 0;

    virtual void SetViewport( float Width, float Height, float MinDepth, float MaxDepth, float x, float y ) = 0;
    virtual void SetScissorRect( float Width, float Height, float x, float y ) = 0;

    virtual void SetBlendFactor( const ColorF& Color ) = 0;

    virtual void SetRenderTargets( RenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, DepthStencilView* DepthStencilView ) = 0;

    virtual void SetVertexBuffers( VertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot ) = 0;
    virtual void SetIndexBuffer( IndexBuffer* IndexBuffer ) = 0;

    virtual void SetPrimitiveTopology( EPrimitiveTopology PrimitveTopologyType ) = 0;

    virtual void SetGraphicsPipelineState( class GraphicsPipelineState* PipelineState ) = 0;
    virtual void SetComputePipelineState( class ComputePipelineState* PipelineState ) = 0;

    virtual void Set32BitShaderConstants( Shader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants ) = 0;

    virtual void SetShaderResourceView( Shader* Shader, ShaderResourceView* ShaderResourceView, uint32 ParameterIndex ) = 0;
    virtual void SetShaderResourceViews( Shader* Shader, ShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex ) = 0;

    virtual void SetUnorderedAccessView( Shader* Shader, UnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex ) = 0;
    virtual void SetUnorderedAccessViews( Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex ) = 0;

    virtual void SetConstantBuffer( Shader* Shader, ConstantBuffer* ConstantBuffer, uint32 ParameterIndex ) = 0;
    virtual void SetConstantBuffers( Shader* Shader, ConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex ) = 0;

    virtual void SetSamplerState( Shader* Shader, SamplerState* SamplerState, uint32 ParameterIndex ) = 0;
    virtual void SetSamplerStates( Shader* Shader, SamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex ) = 0;

    virtual void UpdateBuffer( Buffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData ) = 0;
    virtual void UpdateTexture2D( Texture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData ) = 0;

    virtual void ResolveTexture( Texture* Destination, Texture* Source ) = 0;

    virtual void CopyBuffer( Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo ) = 0;
    virtual void CopyTexture( Texture* Destination, Texture* Source ) = 0;
    virtual void CopyTextureRegion( Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo ) = 0;

    virtual void DiscardResource( class Resource* Resource ) = 0;

    virtual void BuildRayTracingGeometry( RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, bool Update ) = 0;
    virtual void BuildRayTracingScene( RayTracingScene* RayTracingScene, const RayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update ) = 0;

    virtual void SetRayTracingBindings(
        RayTracingScene* RayTracingScene,
        RayTracingPipelineState* PipelineState,
        const RayTracingShaderResources* GlobalResource,
        const RayTracingShaderResources* RayGenLocalResources,
        const RayTracingShaderResources* MissLocalResources,
        const RayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources ) = 0;

    virtual void GenerateMips( Texture* Texture ) = 0;

    virtual void TransitionTexture( Texture* Texture, EResourceState BeforeState, EResourceState AfterState ) = 0;
    virtual void TransitionBuffer( Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState ) = 0;

    virtual void UnorderedAccessTextureBarrier( Texture* Texture ) = 0;
    virtual void UnorderedAccessBufferBarrier( Buffer* Buffer ) = 0;

    virtual void Draw( uint32 VertexCount, uint32 StartVertexLocation ) = 0;
    virtual void DrawIndexed( uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation ) = 0;
    virtual void DrawInstanced( uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation ) = 0;

    virtual void DrawIndexedInstanced(
        uint32 IndexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation,
        uint32 StartInstanceLocation ) = 0;

    virtual void Dispatch( uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ ) = 0;

    virtual void DispatchRays(
        RayTracingScene* InScene,
        RayTracingPipelineState* InPipelineState,
        uint32 InWidth,
        uint32 InHeight,
        uint32 InDepth ) = 0;

    virtual void ClearState() = 0;
    virtual void Flush() = 0;

    virtual void InsertMarker( const std::string& Message ) = 0;

    virtual void BeginExternalCapture() = 0;
    virtual void EndExternalCapture() = 0;
};