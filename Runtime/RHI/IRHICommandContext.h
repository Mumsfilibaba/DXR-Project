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

    /* Begins the timestamp with the specified index in the TimestampQuery */
    virtual void BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) = 0;
    /* Ends the timestamp with the specified index in the TimestampQuery */
    virtual void EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) = 0;

    /* Clears a RenderTargetView with a specific color */
    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor) = 0;
    /* Clears a DepthStencilView with a depth and stencil value */
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue) = 0;
    /* Clears a UnorderedAccessView with a specific color */
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor) = 0;

    /* Sets the Shading-Rate for the fullscreen */
    virtual void SetShadingRate(EShadingRate ShadingRate) = 0;
    /* Set the Shading-Rate image that should be used */
    virtual void SetShadingRateImage(CRHITexture2D* ShadingImage) = 0;

    // TODO: Implement RenderPasses (For Vulkan)

    /* Begin a RenderPass */
    virtual void BeginRenderPass() = 0;
    /* End the current RenderPass */
    virtual void EndRenderPass() = 0;

    /* Set the current viewport settings */
    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) = 0;
    /* Set the current scissor settings */
    virtual void SetScissorRect(float Width, float Height, float x, float y) = 0;

    /* Set the BlendFactor color */
    virtual void SetBlendFactor(const SColorF& Color) = 0;

    /* Sets all the RenderTargetViews and the DepthStencilView that should be used, nullptr is valid if the view should not be set */
    virtual void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView) = 0;

    /* Set the VertexBuffers */
    virtual void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot) = 0;
    /* Set the IndexBuffer */
    virtual void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer) = 0;

    /* Set the primitive topology */
    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) = 0;

    /* Sets the current graphics PipelineState */
    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) = 0;
    /* Sets the current compute PipelineState */
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) = 0;

    /* Set shader constants */
    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) = 0;

    /* Sets a single ShaderResourceView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) = 0;
    /* Sets multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex) = 0;

    /* Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) = 0;
    /* Sets multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) = 0;

    /* Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    /* Sets multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) = 0;

    /* Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) = 0;
    /* Sets multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) = 0;

    /* Updates the contents of a Buffer */
    virtual void UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    /* Updates the contents of a Texture2D */
    virtual void UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) = 0;

    /* Resolves a multi-sampled texture, must have the same sizes and compatible formats */
    virtual void ResolveTexture(CRHITexture* Destination, CRHITexture* Source) = 0;
    /* Copies the contents from one buffer to another */
    virtual void CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo) = 0;
    /* Copies the entire contents of one texture to another, which require the size and formats to be the same */
    virtual void CopyTexture(CRHITexture* Destination, CRHITexture* Source) = 0;
    /* Copies the region of one texture to another */
    virtual void CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyTextureInfo) = 0;

    /* Destroys a resource, this can be used to not having to deal with resource life time, the resource will be destroyed when the underlying command-list is completed */
    virtual void DestroyResource(class CRHIResource* Resource) = 0;
    /* Signal the driver that the contents can be discarded */
    virtual void DiscardResource(class CRHIMemoryResource* Resource) = 0;

    /* Builds the Bottom Level Acceleration Structure for ray tracing */
    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate) = 0;
    /* Builds the Top Level Acceleration Structure for ray tracing */
    virtual void BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) = 0;

    /* Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) = 0;

    /* Generate mip-levels for a texture. Works with Texture2D and TextureCubes */
    virtual void GenerateMips(CRHITexture* Texture) = 0;

    /* Transition the ResourceState of a texture resource */
    virtual void TransitionTexture(CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState) = 0;
    /* Transition the ResourceState of a buffer resource */
    virtual void TransitionBuffer(CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState) = 0;

    /* Add a UnorderedAccessBarrier for a texture resource, which should be issued before reading of a resource in UnorderedAccessState */
    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) = 0;
    /* Add a UnorderedAccessBarrier for a buffer resource, which should be issued before reading of a resource in UnorderedAccessState */
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) = 0;

    /* Issue a draw-call */
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) = 0;
    /* Issue a draw-call for drawing with an IndexBuffer */
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) = 0;
    /* Issue a draw-call for drawing instanced */
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) = 0;
    /* Issue a draw-call for drawing instanced with an IndexBuffer */
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) = 0;

    /* Issues a compute dispatch */
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    /* Issues a ray generation dispatch */
    virtual void DispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) = 0;

    /* Clears the state of the context, clearing all bound references currently bound */
    virtual void ClearState() = 0;

    /* Waits for all current execution on the GPU to finish */
    virtual void Flush() = 0;

    /* Inserts a marker on the GPU timeline */
    virtual void InsertMarker(const CString& Message) = 0;

    /* Begins a PIX capture event, currently only available on D3D12 */
    virtual void BeginExternalCapture() = 0;
    /* Ends a PIX capture event, currently only available on D3D12 */
    virtual void EndExternalCapture() = 0;
};