#pragma once
#include "RenderingCore.h"
#include "Resources.h"
#include "ResourceViews.h"

#include <Containers/ArrayView.h>

class ICommandContext : public RefCountedObject
{
public:
    virtual void Begin() = 0;
    virtual void End()   = 0;

    virtual void ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorF& ClearColor) = 0;
    virtual void ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue) = 0;
    virtual void ClearUnorderedAccessViewFloat(UnorderedAccessView* UnorderedAccessView, const Float ClearColor[4]) = 0;

    virtual void SetShadingRate(EShadingRate ShadingRate) = 0;
    virtual void SetShadingRateImage(Texture2D* ShadingImage) = 0;

    virtual void BeginRenderPass() = 0;
    virtual void EndRenderPass()   = 0;

    virtual void SetViewport(Float Width, Float Height, Float MinDepth, Float MaxDepth, Float x, Float y) = 0;
    virtual void SetScissorRect(Float Width, Float Height, Float x, Float y) = 0;

    virtual void SetBlendFactor(const ColorF& Color) = 0;

    virtual void SetRenderTargets(RenderTargetView* const * RenderTargetViews, UInt32 RenderTargetCount, DepthStencilView* DepthStencilView) = 0;

    virtual void SetVertexBuffers(VertexBuffer* const * VertexBuffers, UInt32 BufferCount, UInt32 BufferSlot) = 0;
    virtual void SetIndexBuffer(IndexBuffer* IndexBuffer) = 0;

    virtual void SetHitGroups(RayTracingScene* Scene, RayTracingPipelineState* PipelineState, const TArrayView<RayTracingShaderResources>& LocalShaderResources) = 0;

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) = 0;

    virtual void SetGraphicsPipelineState(class GraphicsPipelineState* PipelineState) = 0;
    virtual void SetComputePipelineState(class ComputePipelineState* PipelineState)   = 0;

    virtual void Set32BitShaderConstants(Shader* Shader, const Void* Shader32BitConstants, UInt32 Num32BitConstants) = 0;
    
    virtual void SetShaderResourceView(Shader* Shader, ShaderResourceView* ShaderResourceView, UInt32 ParameterIndex) = 0;
    virtual void SetShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceView, UInt32 NumShaderResourceViews, UInt32 ParameterIndex) = 0;

    virtual void SetUnorderedAccessView(Shader* Shader, UnorderedAccessView* UnorderedAccessView, UInt32 ParameterIndex) = 0;
    virtual void SetUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, UInt32 NumUnorderedAccessViews, UInt32 ParameterIndex) = 0;

    virtual void SetConstantBuffer(Shader* Shader, ConstantBuffer* ConstantBuffer, UInt32 ParameterIndex) = 0;
    virtual void SetConstantBuffers(Shader* Shader, ConstantBuffer* const* ConstantBuffers, UInt32 NumConstantBuffers, UInt32 ParameterIndex) = 0;

    virtual void SetSamplerState(Shader* Shader, SamplerState* SamplerState, UInt32 ParameterIndex) = 0;
    virtual void SetSamplerStates(Shader* Shader, SamplerState* const* SamplerStates, UInt32 NumSamplerStates, UInt32 ParameterIndex) = 0;

    virtual void UpdateBuffer(Buffer* Destination, UInt64 OffsetInBytes, UInt64 SizeInBytes, const Void* SourceData) = 0;
    virtual void UpdateTexture2D(Texture2D* Destination, UInt32 Width, UInt32 Height, UInt32 MipLevel, const Void* SourceData) = 0;

    virtual void ResolveTexture(Texture* Destination, Texture* Source) = 0;

    virtual void CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo) = 0;
    virtual void CopyTexture(Texture* Destination, Texture* Source) = 0;
    virtual void CopyTextureRegion(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo) = 0;

    virtual void DiscardResource(class Resource* Resource) = 0;

    virtual void BuildRayTracingGeometry(RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, Bool Update) = 0;
    virtual void BuildRayTracingScene(RayTracingScene* RayTracingScene, TArrayView<RayTracingGeometryInstance> Instances, Bool Update) = 0;

    virtual void GenerateMips(Texture* Texture) = 0;

    virtual void TransitionTexture(Texture* Texture, EResourceState BeforeState, EResourceState AfterState) = 0;
    virtual void TransitionBuffer(Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState) = 0;

    virtual void UnorderedAccessTextureBarrier(Texture* Texture) = 0;
    virtual void UnorderedAccessBufferBarrier(Buffer* Buffer) = 0;

    virtual void Draw(UInt32 VertexCount, UInt32 StartVertexLocation) = 0;
    virtual void DrawIndexed(UInt32 IndexCount, UInt32 StartIndexLocation, UInt32 BaseVertexLocation) = 0;
    virtual void DrawInstanced(UInt32 VertexCountPerInstance, UInt32 InstanceCount, UInt32 StartVertexLocation, UInt32 StartInstanceLocation) = 0;
    
    virtual void DrawIndexedInstanced(
        UInt32 IndexCountPerInstance, 
        UInt32 InstanceCount, 
        UInt32 StartIndexLocation, 
        UInt32 BaseVertexLocation, 
        UInt32 StartInstanceLocation) = 0;

    virtual void Dispatch(UInt32 WorkGroupsX, UInt32 WorkGroupsY, UInt32 WorkGroupsZ) = 0;
    
    virtual void DispatchRays(
        RayTracingScene* InScene,
        Texture2D* InOutputImage,
        RayTracingPipelineState* InPipelineState,
        const RayTracingShaderResources& InGlobalShaderResources,
        UInt32 InWidth,
        UInt32 InHeight,
        UInt32 InDepth) = 0;

    virtual void ClearState() = 0;
    virtual void Flush()      = 0;

    virtual void InsertMarker(const std::string& Message) = 0;

    virtual void BeginExternalCapture() = 0;
    virtual void EndExternalCapture()   = 0;
};