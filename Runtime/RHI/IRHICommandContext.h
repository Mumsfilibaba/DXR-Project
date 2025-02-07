#pragma once
#include "RHI/RHITypes.h"
#include "RHI/RHIResources.h"

class FRHIViewport;
class FRHIRayTracingGeometry;
class FRHIRayTracingScene;
class FRHIQuery;
class FRHIShader;
class FRHIRayTracingPipelineState;
struct FRayTracingShaderResources;
struct FRHIRayTracingGeometryInstance;

struct IRHICommandContext
{
    /** 
     * @brief Begin Frame on the RHIThread
     */
    virtual void RHIBeginFrame() = 0;
    
    /**
     * @brief End Frame on the RHIThread
     */
    virtual void RHIEndFrame() = 0;

    /**
     * @brief Prepares the context to records commands
     */
    virtual void RHIStartContext() = 0;

    /**
     * @brief Ends recording of commands for the context and submits the commandlists
     */
    virtual void RHIFinishContext() = 0;

    /**
     * @brief Begins a query 
     * @param Query Query object to store the query in
     */
    virtual void RHIBeginQuery(FRHIQuery* Query) = 0;

    /**
     * @brief Ends a query
     * @param Query Query object to store the query in
     */
    virtual void RHIEndQuery(FRHIQuery* Query) = 0;

    /**
     * @brief Inserts a timestamp in the query 
     * @param Query Query object to store the query in
     */ 
    virtual void RHIQueryTimestamp(FRHIQuery* Query) = 0;

    /**
     * @brief Clears a RenderTargetView with a specific color 
     * @param RenderTargetView RenderTargetView to clear
     * @param ClearColor Color to set each pixel within the RenderTargetView to
     */
    virtual void RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) = 0;

    /**
     * @brief Clears a DepthStencilView with a specific value
     * @param DepthStencilView DepthStencilView to clear
     * @param ClearValue Value to set each pixel within the DepthStencilView to
     */
    virtual void RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil) = 0;
    
    /**
     * @brief Clears a UnorderedAccessView with a specific value
     * @param UnorderedAccessView UnorderedAccessView to clear
     * @param ClearColor Value to set each pixel within the UnorderedAccessView to
     */
    virtual void RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) = 0;

    /**
     * @brief Begins a new RenderPass
     * @param BeginRenderPassInfo Description of RenderTargets and DepthStencils to bind for drawing
     */
    virtual void RHIBeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) = 0;

    /**
     * @brief Ends the current RenderPass
     */
    virtual void RHIEndRenderPass() = 0;

    /**
     * @brief Set the current viewport settings
     * @param ViewportRegion Region of the viewport
     */
    virtual void RHISetViewport(const FViewportRegion& ViewportRegion) = 0;
    
    /**
     * @brief Set the current scissor settings 
     * @param ScissorRegion Region of the scissor rectangle
     */
    virtual void RHISetScissorRect(const FScissorRegion& ScissorRegion) = 0;

    /**
     * @brief Set the BlendFactor color 
     * @param Color New blend-factor to use
     */
    virtual void RHISetBlendFactor(const FVector4& Color) = 0;

    /**
     * @brief Set the VertexBuffers to be used
     * @param VertexBuffers ArrayView of VertexBuffers to use
     * @param BufferSlot Slot to start bind the array to
     */
    virtual void RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) = 0;
    
    /**
     * @brief Set the current IndexBuffer 
     * @param IndexBuffer IndexBuffer to use
     * @param IndexFormat Format of the indices in the IndexBuffer
     */
    virtual void RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) = 0;

    /**
     * @brief Sets the current graphics PipelineState 
     * @param PipelineState New PipelineState to use
     */
    virtual void RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) = 0;
    
    /**
     * @brief Sets the current compute PipelineState
     * @param PipelineState New PipelineState to use
     */
    virtual void RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState) = 0;

    /**
     * @brief Set shader constants
     * @param Shader Shader to bind the constants to
     * @param Shader32BitConstants Array of 32-bit constants
     * @param Num32bitConstants Number o 32-bit constants (Each is 4 bytes)
     */
    virtual void RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) = 0;

    /**
     * @brief Sets a single ShaderResourceView to the ParameterIndex this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param ShaderResourceView ShaderResourceView to bind
     * @param ParameterIndex ShaderResourceView-index to bind to
     */
    virtual void RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) = 0;
    
    /**
     * @brief Sets a multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param ShaderResourceViews ArrayView of ShaderResourceViews to bind
     * @param ParameterIndex ShaderResourceView-index to bind to
     */
    virtual void RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) = 0;

    /**
     * @brief Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param UnorderedAccessView UnorderedAccessView to bind
     * @param ParameterIndex UnorderedAccessView-index to bind to
     */
    virtual void RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) = 0;

    /**
     * @brief Sets a multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param InUnorderedAccessViews ArrayView of UnorderedAccessViews to bind
     * @param ParameterIndex UnorderedAccessView-index to bind to
     */
    virtual void RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) = 0;

    /**
     * @brief Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param ConstantBuffer ConstantBuffer to bind
     * @param ParameterIndex ConstantBuffer-index to bind to
     */
    virtual void RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    
    /**
     * @brief Sets a multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param ConstantBuffers ArrayView of ConstantBuffers to bind
     * @param ParameterIndex ConstantBuffer-index to bind to
     */
    virtual void RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) = 0;

    /**
     * @brief Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind sampler to
     * @param SamplerState SamplerState to bind
     * @param ParameterIndex SamplerState-index to bind to
     */
    virtual void RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) = 0;

    /**
     * @brief Sets a multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object.
     * @param Shader Shader to bind resource to
     * @param SamplerStates ArrayView of SamplerStates to bind
     * @param ParameterIndex ConstantBuffer-index to bind to
     */
    virtual void RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) = 0;

    /**
     * @brief Updates the contents of a Buffer
     * @param Dst Destination buffer to update
     * @param BufferRegion - BufferRegion to copy
     * @param SrcData SrcData to copy to the GPU
     */
    virtual void RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) = 0;
    
    /**
     * @brief Updates the contents of a Texture2D
     * @param Dst Destination Texture2D to update
     * @param TextureRegion Describes the region of the texture to copy
     * @param MipLevel MipLevel of the texture to update
     * @param SrcData SrcData to copy to the GPU
     * @param SrcRowPitch RowPitch of the SrcData
     */
    virtual void RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) = 0;

    /**
     * @brief Resolves a multi-sampled texture, must have the same sizes and compatible formats
     * @param Dst Destination texture, must have a single sample
     * @param Src Source texture to resolve
     */
    virtual void RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src) = 0;
    
    /**
     * @brief Copies the contents from one buffer to another 
     * @param Dst Destination buffer to copy to
     * @param Src Source buffer to copy from
     * @param CopyDesc Information about the copy operation
     */
    virtual void RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) = 0;
    
    /**
     * @brief Copies the entire contents of one texture to another, which require the size and formats to be the same 
     * @param Dst Destination texture
     * @param Src Source texture
     */ 
    virtual void RHICopyTexture(FRHITexture* Dst, FRHITexture* Src) = 0;

    /**
     * @brief Copies contents of a texture region of one texture to another, which require the size and formats to be the same.
     * @param Dst Destination texture
     * @param Src Source texture
     * @param CopyDesc Information about the copy operation
     */
    virtual void RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) = 0;

    /**
     * @brief Signal the driver that the contents can be discarded
     * @param Texture Resource to discard contents of
     */
    virtual void RHIDiscardContents(class FRHITexture* Texture) = 0;

    /**
     * @brief Builds the Top-Level Acceleration-Structure for ray tracing
     * @param RayTracingScene Top-level acceleration-structure to build or update
     * @param BuildInfo A structure containing information about the build
     */
    virtual void RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) = 0;

    /**
     * @brief Builds the Bottom-Level Acceleration-Structure for ray tracing 
     * @param RayTracingGeometry Bottom-level acceleration-structure to build or update
     * @param BuildInfo A structure containing information about the build
     */ 
    virtual void RHIBuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) = 0;

     /** 
      * @brief Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored
      */
    virtual void RHISetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) = 0;

    /**
     * @brief Generate MipLevels for a texture. Works with Texture2D and TextureCubes.
     * @param Texture Texture to generate MipLevels for
     */
    virtual void RHIGenerateMips(FRHITexture* Texture) = 0;

    /**
     * @brief Transition the ResourceState of a Texture resource.
     * @param Texture Texture to transition ResourceState for
     * @param BeforeState State that the Texture had before the transition
     * @param AfterState State that the Texture have after the transition
     */
    virtual void RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;
    
    /**
     * @brief Transition the ResourceState of a Buffer resource
     * @param Buffer Buffer to transition ResourceState for
     * @param BeforeState State that the Buffer had before the transition
     * @param AfterState State that the Buffer have after the transition
     */
    virtual void RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;

    /**
     * @brief Add a UnorderedAccessBarrier for a Texture resource, which should be issued before reading of a resource in UnorderedAccessState.
     * @param Texture Texture to issue barrier for
     */
    virtual void RHIUnorderedAccessTextureBarrier(FRHITexture* Texture) = 0;
    
    /**
     * @brief Add a UnorderedAccessBarrier for a Buffer resource, which should be issued before reading of a resource in UnorderedAccessState.
     * @param Buffer Buffer to issue barrier for
     */
    virtual void RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer) = 0;

    virtual void RHIDraw(uint32 VertexCount, uint32 StartVertexLocation) = 0;
    virtual void RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) = 0;
    virtual void RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) = 0;
    virtual void RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) = 0;
    virtual void RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;
    virtual void RHIDispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) = 0;
    virtual void RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync) = 0;
    virtual void RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height) = 0;

    /**
     * @brief Clears the state of the context, clearing all bound references currently bound
     */
    virtual void RHIClearState() = 0;

    /** 
     * @brief Waits for all current execution on the GPU to finish
     */
    virtual void RHIFlush() = 0;

    /**
     * @brief Inserts a marker on the GPU timeline 
     */
    virtual void RHIInsertMarker(const FStringView& Message) = 0;

    /**
     * @brief Begins a PIX capture event, currently only available on D3D12
     */
    virtual void RHIBeginExternalCapture() = 0;
    
    /**
     * @brief Ends a PIX capture event, currently only available on D3D12
     */
    virtual void RHIEndExternalCapture() = 0;

    /**
     * @return Returns the native CommandList
     */
    virtual void* RHIGetNativeCommandList() = 0;
};
