#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"

#include "Core/Containers/ArrayView.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext : public CRefCounted
{
public:

    /**
     * Start recording commands with this command context 
     */
    virtual void Begin() = 0;
    
    /**
     * Stop recording commands with this command context
     */
    virtual void End() = 0;

    /**
     * Begins the timestamp with the specified index in the TimestampQuery 
     * 
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to begin
     */ 
    virtual void BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) = 0;
    
    /**
     * Ends the timestamp with the specified index in the TimestampQuery
     *
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to end
     */
    virtual void EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) = 0;

    /**
     * Clears a RenderTargetView with a specific color 
     * 
     * @param RenderTargetView: RenderTargetView to clear
     * @param ClearColor: Color to set each pixel within the RenderTargetView to
     */
    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor) = 0;

    /**
     * Clears a DepthStencilView with a specific value
     *
     * @param DepthStencilView: DepthStencilView to clear
     * @param ClearValue: Value to set each pixel within the DepthStencilView to
     */
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue) = 0;
    
    /**
     * Clears a UnorderedAccessView with a specific value
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Value to set each pixel within the UnorderedAccessView to
     */
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor) = 0;

    /**
     * Sets the Shading-Rate for the fullscreen
     * 
     * @param ShadingRate: New shading-rate for the upcoming draw-calls
     */
    virtual void SetShadingRate(EShadingRate ShadingRate) = 0;

    /**
     * Set the Shading-Rate image that should be used 
     * 
     * @param ShadingImage: Image containing the shading rate for the next upcoming draw-calls
     */
    virtual void SetShadingRateImage(CRHITexture2D* ShadingImage) = 0;

    /**
     * Begin a RenderPass 
     */
    virtual void BeginRenderPass() = 0;

    /**
     * Ends a RenderPass
     */
    virtual void EndRenderPass() = 0;

    /**
     * Set the current viewport settings
     * 
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param MinDepth: Minimum-depth of the viewport
     * @param MaxDepth: Maximum-depth of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) = 0;
    
    /**
     * Set the current scissor settings 
     * 
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    virtual void SetScissorRect(float Width, float Height, float x, float y) = 0;

    /**
     * Set the BlendFactor color 
     * 
     * @param Color: New blend-factor to use
     */
    virtual void SetBlendFactor(const SColorF& Color) = 0;

    /**
     * Set all the RenderTargetViews and the DepthStencilView that should be used, nullptr is valid if the slot should not be used
     * 
     * @param RenderTargetViews: Array of RenderTargetViews to use, each pointer in the array must be valid
     * @param RenderTargetCount: Number of RenderTargetViews in the array
     * @param DepthStencilView: DepthStencilView to set
     */ 
    virtual void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView) = 0;

    /**
     * Set the VertexBuffers to be used
     * 
     * @param VertexBuffers: Array of VertexBuffers to use
     * @param VertexBufferCount: Number of VertexBuffers in the array
     * @param BufferSlot: Slot to start bind the array to
     */
    virtual void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot) = 0;
    
    /**
     * Set the current IndexBuffer 
     * 
     * @param IndexBuffer: IndexBuffer to use
     */
    virtual void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer) = 0;

    /**
     * Set the primitive topology 
     * 
     * @param PrimitveTopologyType: New primitive topology to use
     */
    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) = 0;

    /**
     * Sets the current graphics PipelineState 
     * 
     * @param PipelineState: New PipelineState to use
     */
    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) = 0;
    
    /**
     * Sets the current compute PipelineState
     *
     * @param PipelineState: New PipelineState to use
     */
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) = 0;

    /**
     * Set shader constants
     * 
     * @param Shader: Shader to bind the constants to
     * @param Shader32BitConstants: Array of 32-bit constants
     * @param Num32bitConstants: Number o 32-bit constants (Each is 4 bytes)
     */
    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) = 0;

    /**
     * Sets a single ShaderResourceView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object 
     * 
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceView: ShaderResourceView to bind
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) = 0;
    
    /**
     * Sets a multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceViews: Array of ShaderResourceViews to bind
     * @param NumShaderResourceViews: Number of ShaderResourceViews in the array
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex) = 0;

    /**
     * Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessView: UnorderedAccessView to bind
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) = 0;

    /**
     * Sets a multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessViews: Array of UnorderedAccessViews to bind
     * @param NumUnorderedAccessViews: Number of UnorderedAccessViews in the array
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) = 0;

    /**
     * Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffer: ConstantBuffer to bind
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    
    /**
     * Sets a multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffers: Array of ConstantBuffers to bind
     * @param NumConstantBuffers: Number of ConstantBuffers in the array
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) = 0;

    /**
     * Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind sampler to
     * @param SamplerState: SamplerState to bind
     * @param ParameterIndex: SamplerState-index to bind to
     */
    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) = 0;

    /**
     * Sets a multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param SamplerStates: Array of SamplerStates to bind
     * @param NumConstantBuffers: Number of ConstantBuffers in the array
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) = 0;

    /**
     * Updates the contents of a Buffer
     * 
     * @param Dst: Destination buffer to update
     * @param OffsetInBytes: Offset in bytes inside the destination-buffer
     * @param SizeInBytes: Number of bytes to copy over to the buffer
     * @param SourceData: SourceData to copy to the GPU
     */
    virtual void UpdateBuffer(CRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    
    /**
     * Updates the contents of a Texture2D
     *
     * @param Dst: Destination Texture2D to update
     * @param Width: Width of the texture to update
     * @param Height: Height of the texture to update
     * @param MipLevel: MipLevel of the texture to update
     * @param SourceData: SourceData to copy to the GPU
     */
    virtual void UpdateTexture2D(CRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) = 0;

    /**
     * Resolves a multi-sampled texture, must have the same sizes and compatible formats
     * 
     * @param Dst: Destination texture, must have a single sample
     * @param Src: Source texture to resolve
     */
    virtual void ResolveTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    
    /**
     * Copies the contents from one buffer to another 
     * 
     * @param Dst: Destination buffer to copy to
     * @param Src: Source buffer to copy from
     * @param CopyInfo: Information about the copy operation
     */
    
    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SCopyBufferInfo& CopyInfo) = 0;
    
    /**
     * Copies the entire contents of one texture to another, which require the size and formats to be the same 
     * 
     * @param Dst: Destination texture
     * @param Src: Source texture
     */ 
    virtual void CopyTexture(CRHITexture* Dst, CRHITexture* Src) = 0;

    /**
     * Copies contents of a texture region of one texture to another, which require the size and formats to be the same
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     * @param CopyTextureInfo: Information about the copy operation
     */
    virtual void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SCopyTextureInfo& CopyTextureInfo) = 0;

    /**
     * Destroys a resource, this can be used to not having to deal with resource life time, the resource will be destroyed when the underlying command-list is completed
     * 
     * @param Resource: Resource to destroy
     */
    virtual void DestroyResource(class CRHIObject* Resource) = 0;

    /**
     * Signal the driver that the contents can be discarded
     *
     * @param Resource: Resource to discard contents of
     */
    virtual void DiscardContents(class CRHIResource* Resource) = 0;

    /**
     * Builds the Bottom-Level Acceleration-Structure for ray tracing 
     * 
     * @param Geometry: Bottom-level acceleration-structure to build or update
     * @param VertexBuffer: VertexBuffer to build Geometry of
     * @param IndexBuffer: IndexBuffer to build Geometry of
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */ 
    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate) = 0;
    
    /**
     * Builds the Top-Level Acceleration-Structure for ray tracing
     *
     * @param Scene: Top-level acceleration-structure to build or update
     * @param Instances: Instances to build the scene of
     * @param NumInstances: Number of instances to build
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) = 0;

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