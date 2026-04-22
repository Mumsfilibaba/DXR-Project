#pragma once
#include "RHI/RHITypes.h"
#include "RHI/RHIResources.h"

class FRHISwapChain;
class FRHIRayTracingGeometry;
class FRHIRayTracingScene;
class FRHIQuery;
class FRHIShader;
class FRHIRayTracingPipelineState;
class FRHIGpuFence;
struct FRayTracingShaderResources;
struct FRHIRayTracingGeometryInstance;
struct FRHITextureTransition;

enum class ECommandContextPhase
{
    Finished = 0,
    Recording,
    InsideRenderPass,
    RenderPassPaused,
};

struct IRHICommandContext
{
    /**
     * @brief Begin Frame on the RHIThread
     */
    virtual void BeginFrame() = 0;

    /**
     * @brief End Frame on the RHIThread
     */
    virtual void EndFrame() = 0;

    /**
     * @brief Prepares the context to records commands
     */
    virtual void StartContext() = 0;

    /**
     * @brief Ends recording of commands for the context and submits the commandlists
     */
    virtual void FinishContext() = 0;

    /**
     * @brief Begins a query
     * @param Query Query object to store the query in
     */
    virtual void BeginQuery(FRHIQuery* Query) = 0;

    /**
     * @brief Ends a query
     * @param Query Query object to store the query in
     */
    virtual void EndQuery(FRHIQuery* Query) = 0;

    /**
     * @brief Inserts a timestamp in the query
     * @param Query Query object to store the query in
     */
    virtual void QueryTimestamp(FRHIQuery* Query) = 0;

    /**
     * @brief Clears a RenderTargetView with a specific color
     * @param RenderTargetView RenderTargetView to clear
     * @param ClearColor Color to set each pixel within the RenderTargetView to
     */
    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) = 0;

    /**
     * @brief Clears a DepthStencilView with a specific value
     * @param DepthStencilView DepthStencilView to clear
     * @param ClearValue Value to set each pixel within the DepthStencilView to
     */
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil) = 0;

    /**
     * @brief Clears a UnorderedAccessView with float values
     * @param UnorderedAccessView UnorderedAccessView to clear
     * @param ClearColor Value to set each pixel within the UnorderedAccessView to
     */
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) = 0;

    /**
     * @brief Clears a UnorderedAccessView with unsigned integer values
     * @param UnorderedAccessView UnorderedAccessView to clear
     * @param Values Four uint32 values to set each element within the UnorderedAccessView to
     */
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) = 0;

    /**
     * @brief Begins a new RenderPass
     * @param BeginRenderPassInfo Description of RenderTargets and DepthStencils to bind for drawing
     */
    virtual void BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) = 0;

    /**
     * @brief Ends the current RenderPass
     */
    virtual void EndRenderPass() = 0;

    /**
     * @brief Set the current viewport settings
     * @param ViewportRegion Region of the viewport
     */
    virtual void SetViewport(const FViewportRegion& ViewportRegion) = 0;

    /**
     * @brief Set the current scissor settings
     * @param ScissorRegion Region of the scissor rectangle
     */
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) = 0;

    /**
     * @brief Set the BlendFactor color
     * @param Color New blend-factor to use
     */
    virtual void SetBlendFactor(const FVector4& Color) = 0;

    /**
     * @brief Set the stencil reference value
     * @param StencilRef New stencil reference value
     */
    virtual void SetStencilRef(uint32 StencilRef) = 0;

    /**
     * @brief Set the depth bias parameters dynamically (requires RHIDeviceFeatureSupport::bSupportsDynamicDepthBias)
     * @param DepthBias Constant depth bias
     * @param DepthBiasClamp Maximum depth bias clamp
     * @param SlopeScaledDepthBias Slope-scaled depth bias
     */
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) = 0;

    /**
     * @brief Set the VertexBuffers to be used
     * @param VertexBuffers ArrayView of VertexBuffers to use
     * @param BufferSlot Slot to start bind the array to
     */
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) = 0;

    /**
     * @brief Set the current IndexBuffer
     * @param IndexBuffer IndexBuffer to use
     * @param IndexFormat Format of the indices in the IndexBuffer
     */
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) = 0;

    /**
     * @brief Set the stream output target buffers
     * @param Buffers ArrayView of stream output target buffers
     * @param Offsets Per-buffer byte offsets into each target
     */
    virtual void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets) = 0;

    /**
     * @brief Sets the current graphics PipelineState
     * @param PipelineState New PipelineState to use
     */
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) = 0;

    /**
     * @brief Sets the current compute PipelineState
     * @param PipelineState New PipelineState to use
     */
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) = 0;

    /**
     * @brief Set shader constants
     * @param Shader Shader to bind the constants to
     * @param ShaderConstants Array of 32-bit constants
     * @param NumShaderConstants Number o 32-bit constants (Each is 4 bytes)
     */
    virtual void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) = 0;

    /**
     * @brief Sets a single ShaderResourceView at the specified register index. RegisterIndex corresponds to the HLSL register (e.g., register(t0)).
     * @param Shader Shader to bind resource to
     * @param ShaderResourceView ShaderResourceView to bind
     * @param RegisterIndex Register index to bind to
     */
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets multiple ShaderResourceViews starting at the specified register index (for arrays in the shader). RegisterIndex corresponds to the HLSL register (e.g., register(t0)).
     * @param Shader Shader to bind resource to
     * @param ShaderResourceViews ArrayView of ShaderResourceViews to bind
     * @param RegisterIndex Starting register index to bind from
     */
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets a single UnorderedAccessView at the specified register index. RegisterIndex corresponds to the HLSL register (e.g., register(u0)).
     * @param Shader Shader to bind resource to
     * @param UnorderedAccessView UnorderedAccessView to bind
     * @param RegisterIndex Register index to bind to
     */
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets multiple UnorderedAccessViews starting at the specified register index (for arrays in the shader). RegisterIndex corresponds to the HLSL register (e.g., register(u0)).
     * @param Shader Shader to bind resource to
     * @param InUnorderedAccessViews ArrayView of UnorderedAccessViews to bind
     * @param RegisterIndex Starting register index to bind from
     */
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets a single ConstantBuffer at the specified register index. RegisterIndex corresponds to the HLSL register (e.g., register(b0)).
     * @param Shader Shader to bind resource to
     * @param ConstantBuffer ConstantBuffer to bind
     * @param RegisterIndex Register index to bind to
     */
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets multiple ConstantBuffers starting at the specified register index (for arrays in the shader). RegisterIndex corresponds to the HLSL register (e.g., register(b0)).
     * @param Shader Shader to bind resource to
     * @param ConstantBuffers ArrayView of ConstantBuffers to bind
     * @param RegisterIndex Starting register index to bind from
     */
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets a single SamplerState at the specified register index. RegisterIndex corresponds to the HLSL register (e.g., register(s0)).
     * @param Shader Shader to bind sampler to
     * @param SamplerState SamplerState to bind
     * @param RegisterIndex Register index to bind to
     */
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex) = 0;

    /**
     * @brief Sets multiple SamplerStates starting at the specified register index (for arrays in the shader). RegisterIndex corresponds to the HLSL register (e.g., register(s0)).
     * @param Shader Shader to bind resource to
     * @param SamplerStates ArrayView of SamplerStates to bind
     * @param RegisterIndex Starting register index to bind from
     */
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex) = 0;

    /**
     * @brief Updates the contents of a Buffer
     * @param Dst Destination buffer to update
     * @param BufferRegion - BufferRegion to copy
     * @param SrcData SrcData to copy to the GPU
     */
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) = 0;

    /**
     * @brief Updates the contents of a Texture2D
     * @param Dst Destination Texture2D to update
     * @param TextureRegion Describes the region of the texture to copy
     * @param MipLevel MipLevel of the texture to update
     * @param SrcData SrcData to copy to the GPU
     * @param SrcRowPitch RowPitch of the SrcData
     */
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) = 0;

    /**
     * @brief Updates a region of a 3D texture (or any texture with depth/array slices).
     * @param Dst Destination texture
     * @param TextureRegion 3D region describing width, height, depth, and offsets
     * @param MipLevel Mip level to update
     * @param SrcData Source data pointer
     * @param SrcRowPitch Byte stride between rows in SrcData
     * @param SrcDepthPitch Byte stride between depth slices in SrcData
     */
    virtual void UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch) = 0;

    /**
     * @brief Resolves a multi-sampled texture, must have the same sizes and compatible formats
     * @param Dst Destination texture, must have a single sample
     * @param Src Source texture to resolve
     */
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) = 0;

    /**
     * @brief Copies the contents from one buffer to another
     * @param Dst Destination buffer to copy to
     * @param Src Source buffer to copy from
     * @param CopyDesc Information about the copy operation
     */
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) = 0;

    /**
     * @brief Copies the entire contents of one texture to another, which require the size and formats to be the same
     * @param Dst Destination texture
     * @param Src Source texture
     */
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) = 0;

    /**
     * @brief Copies contents of a texture region of one texture to another, which require the size and formats to be the same.
     * @param Dst Destination texture
     * @param Src Source texture
     * @param CopyDesc Information about the copy operation
     */
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) = 0;

    /**
     * @brief Copies a 2D region from a texture to a buffer (typically readback).
     * @param Dst Destination buffer.
     * @param DstOffset Offset into destination buffer (bytes).
     * @param Src Source texture.
     * @param SrcRegion Source region (texel coordinates).
     * @param SrcMipLevel Source mip level.
     */
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) = 0;

    /**
     * @brief Copies a 3D region from a specific subresource of a texture to a buffer (typically readback).
     * @param Dst Destination buffer
     * @param DstOffset Offset into destination buffer (bytes)
     * @param Src Source texture
     * @param SrcRegion Source region (3D texel coordinates)
     * @param SrcMipLevel Source mip level
     * @param SrcArraySlice Source array slice (0 for 3D textures)
     */
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) = 0;

    /**
     * @brief Splits the command stream and signals the provided fence after all prior GPU work is complete.
     */
    virtual void WriteFence(FRHIGpuFence* Fence) = 0;

    /**
     * @brief Signal the driver that the contents can be discarded
     * @param Texture Resource to discard contents of
     */
    virtual void DiscardContents(class FRHITexture* Texture) = 0;

    /**
     * @brief Builds the Top-Level Acceleration-Structure for ray tracing
     * @param RayTracingScene Top-level acceleration-structure to build or update
     * @param BuildInfo A structure containing information about the build
     */
    virtual void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) = 0;

    /**
     * @brief Builds the Bottom-Level Acceleration-Structure for ray tracing
     * @param RayTracingGeometry Bottom-level acceleration-structure to build or update
     * @param BuildInfo A structure containing information about the build
     */
    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) = 0;

    /**
     * @brief Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored
     */
    virtual void SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) = 0;

    /**
     * @brief Transition the ResourceState of a Texture resource.
     * @param Texture Texture to transition ResourceState for
     * @param TextureTransition Part of the texture to transition
     */
    virtual void TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) = 0;

    /**
     * @brief Transition the ResourceState of a Buffer resource
     * @param Buffer Buffer to transition ResourceState for
     * @param BeforeState State that the Buffer had before the transition
     * @param AfterState State that the Buffer have after the transition
     */
    virtual void TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;

    /**
     * @brief Ensure a Texture resource is in the required state. The before-state is inferred from the tracked state.
     * @param Texture Texture to transition
     * @param RequiredState The state the texture must be in
     */
    virtual void RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState) = 0;

    /**
     * @brief Ensure a Buffer resource is in the required state. The before-state is inferred from the tracked state.
     * @param Buffer Buffer to transition
     * @param RequiredState The state the buffer must be in
     */
    virtual void RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState) = 0;

    /**
     * @brief Add a UnorderedAccessBarrier for a Texture resource, which should be issued before reading of a resource in UnorderedAccessState.
     * @param Texture Texture to issue barrier for
     */
    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) = 0;

    /**
     * @brief Add a UnorderedAccessBarrier for a Buffer resource, which should be issued before reading of a resource in UnorderedAccessState.
     * @param Buffer Buffer to issue barrier for
     */
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) = 0;

    /**
     * @brief Draws primitives using a non-indexed geometry.
     * @param VertexCount Number of vertices to draw.
     * @param StartVertexLocation Starting vertex location.
     */
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) = 0;

    /**
     * @brief Draws primitives using indexed geometry.
     * @param IndexCount Number of indices to draw.
     * @param StartIndexLocation Starting index location.
     * @param BaseVertexLocation Base vertex location.
     */
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) = 0;

    /**
     * @brief Draws instanced primitives using non-indexed geometry.
     * @param VertexCountPerInstance Number of vertices per instance.
     * @param InstanceCount Number of instances to draw.
     * @param StartVertexLocation Starting vertex location.
     * @param StartInstanceLocation Starting instance location.
     */
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) = 0;

    /**
     * @brief Draws instanced primitives using indexed geometry.
     * @param IndexCountPerInstance Number of indices per instance.
     * @param InstanceCount Number of instances to draw.
     * @param StartIndexLocation Starting index location.
     * @param BaseVertexLocation Base vertex location.
     * @param StartInstanceLocation Starting instance location.
     */
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) = 0;

    /**
     * @brief Dispatches a compute shader with the specified work group dimensions.
     * @param WorkGroupsX Number of work groups in the X dimension.
     * @param WorkGroupsY Number of work groups in the Y dimension.
     * @param WorkGroupsZ Number of work groups in the Z dimension.
     */
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    /**
     * @brief Dispatches a ray tracing operation.
     * @param Scene Ray tracing scene to use.
     * @param PipelineState Pipeline state for ray tracing.
     * @param Width Dispatch width.
     * @param Height Dispatch height.
     * @param Depth Dispatch depth.
     */
    virtual void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) = 0;

    /**
     * @brief Presents the swap-chain, swapping the back buffer to the screen.
     * @param SwapChain The swap-chain to present.
     * @param bVerticalSync Whether to use vertical synchronization.
     */
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) = 0;

    /**
     * @brief Resizes the specified swap-chain.
     * @param SwapChain The swap-chain to resize.
     * @param Width New width of the swap-chain.
     * @param Height New height of the swap-chain.
     */
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height) = 0;

    /**
     * @brief Clears the state of the context, clearing all bound references currently bound
     */
    virtual void ClearState() = 0;

    /**
     * @brief Waits for all current execution on the GPU to finish
     */
    virtual void Flush() = 0;

    /**
     * @brief Begins a named GPU event region for profiling tools (PIX, RenderDoc, etc.)
     */
    virtual void PushEvent(const FStringView& Name) = 0;

    /**
     * @brief Ends the current GPU event region
     */
    virtual void PopEvent() = 0;

    /**
     * @brief Returns the native CommandList
     * @return Pointer to the native CommandList
     */
    virtual void* GetNativeCommandList() = 0;
};
