#pragma once
#include "RHI/RHITypes.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIIndirect.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"
#include "RHI/RayTracing/RHIShaderBindingTable.h"

class FRHISwapChain;
class FRHIGeometryAccelerationStructure;
class FRHISceneAccelerationStructure;
class FRHIRayTracingAccelerationStructure;
class FRHIOpacityMicromap;
class FRHIShaderBindingTable;
class FRHIQuery;
class FRHIShader;
class FRHIRayTracingPipelineState;
class FRHIFence;
class FRHIBuffer;
struct FRHIHitGroupLocalShaderBinding;
struct FRHIOpacityMicromapBuildDesc;
struct FRHIRayTracingAccelerationStructureOperationDesc;
struct FRHIGeometryAccelerationStructureInstance;

enum class ECommandContextPhase : uint8
{
    Finished = 0,
    Recording,
    InsideRenderPass,
    RenderPassPaused,
};

enum class ESamplerFeedbackTranscodeMode : uint8
{
    /** Opaque feedback map -> R8_UINT */
    Decode = 0,

    /** R8_UINT -> opaque feedback map */
    Encode = 1,
};

struct IRHICommandContext
{
    virtual ~IRHICommandContext() = default;

    /**
     * @brief Begins a frame on the RHI thread.
     */
    virtual void BeginFrame() = 0;

    /**
     * @brief Ends the current frame on the RHI thread.
     */
    virtual void EndFrame() = 0;

    /**
     * @brief Prepares the context to record commands.
     */
    virtual void StartContext() = 0;

    /**
     * @brief Ends command recording and submits the recorded command lists.
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
    virtual void ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor) = 0;

    /**
     * @brief Clears a depth-stencil view with depth and stencil values.
     * @param DepthStencilView Depth-stencil view to clear.
     * @param Depth Depth value written to the view.
     * @param Stencil Stencil value written to the view.
     */
    virtual void ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, const uint8 Stencil) = 0;

    /**
     * @brief Clears a UnorderedAccessView with float values
     * @param UnorderedAccessView UnorderedAccessView to clear
     * @param ClearColor Value to set each pixel within the UnorderedAccessView to
     */
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor) = 0;

    /**
     * @brief Clears a UnorderedAccessView with unsigned integer values
     * @param UnorderedAccessView UnorderedAccessView to clear
     * @param Values Four uint32 values to set each element within the UnorderedAccessView to
     */
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) = 0;

    /**
     * @brief Begins a new RenderPass
     * @param BeginRenderPassDesc Description of RenderTargets and DepthStencils to bind for drawing
     */
    virtual void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) = 0;

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
    virtual void SetBlendFactor(const Vector4& Color) = 0;

    /**
     * @brief Set the stencil reference value
     * @param StencilRef New stencil reference value
     */
    virtual void SetStencilRef(uint32 StencilRef) = 0;

    /**
     * @brief Set the depth bias parameters dynamically (requires RHI::bSupportsDynamicDepthBias)
     * @param DepthBias Constant depth bias
     * @param DepthBiasClamp Maximum depth bias clamp
     * @param SlopeScaledDepthBias Slope-scaled depth bias
     */
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) = 0;

    /**
     * @brief Set the depth bounds range (requires RHI::bSupportsDepthBoundsTest).
     * @param MinDepth Minimum stored depth that passes the test, in 0..1 depth-buffer space.
     * @param MaxDepth Maximum stored depth that passes the test, in 0..1 depth-buffer space.
     */
    virtual void SetDepthBounds(float MinDepth, float MaxDepth) = 0;

    /**
     * @brief Set custom rasterizer sample positions (requires RHI::bSupportsProgrammableSamplePositions).
     * @param SamplePositionsDesc Positions to use, or NumSamplesPerPixel = 0 to restore the defaults
     */
    virtual void SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc) = 0;

    /**
     * @brief Sets the vertex buffers used by subsequent draw commands.
     * @param InVertexBuffers Vertex buffers to bind.
     * @param BufferSlot First vertex-buffer slot receiving the array.
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
     * @brief Sets the current meshlet (mesh-shader) PipelineState
     * @param PipelineState New PipelineState to use
     */
    virtual void SetMeshletPipelineState(class FRHIMeshletPipelineState* PipelineState) = 0;

    /**
     * @brief Set shader constants
     * @param Shader Shader to bind the constants to
     * @param ShaderConstants Array of 32-bit constants
     * @param NumShaderConstants Number of 32-bit constants in ShaderConstants.
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
     * @param InShaderResourceViews ShaderResourceViews to bind
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
     * @param InConstantBuffers ConstantBuffers to bind
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
     * @param InSamplerStates SamplerStates to bind
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
     * @brief Transcodes between an opaque sampler feedback map and an application-readable R8_UINT texture.
     * @param Dst Destination: the R8_UINT texture when decoding, the feedback map when encoding.
     * @param DstSubresource Destination subresource, or RHI_ALL_SUBRESOURCES.
     * @param Src Source: the feedback map when decoding, the R8_UINT texture when encoding.
     * @param SrcSubresource Source subresource, or RHI_ALL_SUBRESOURCES.
     * @param Mode Whether to decode from or encode into the opaque representation.
     */
    virtual void TranscodeSamplerFeedback(FRHITexture* Dst, uint32 DstSubresource, FRHITexture* Src, uint32 SrcSubresource, ESamplerFeedbackTranscodeMode Mode) = 0;

    /**
     * @brief Copies the contents from one buffer to another
     * @param Dst Destination buffer to copy to
     * @param Src Source buffer to copy from
     * @param CopyDesc Information about the copy operation
     */
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) = 0;

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
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) = 0;

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
     * @param SrcArraySlice Source array slice as a raw 2D layer index (cubeIndex * RHI_NUM_CUBE_FACES + faceIndex for cubes; 0 for 3D textures)
     */
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) = 0;

    /**
     * @brief Splits the command stream and signals the provided fence after all prior GPU work is complete.
     * @param Fence Fence to signal after previously recorded work completes.
     */
    virtual void WriteFence(FRHIFence* Fence) = 0;

    /**
     * @brief Signal the driver that the contents can be discarded
     * @param Texture Resource to discard contents of
     */
    virtual void DiscardContents(class FRHITexture* Texture) = 0;

    /**
     * @brief Builds the Top-Level Acceleration-Structure for ray tracing
     * @param RayTracingScene Top-level acceleration-structure to build or update
     * @param BuildDesc A structure containing information about the build
     */
    virtual void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc) = 0;

    /**
     * @brief Builds the Bottom-Level Acceleration-Structure for ray tracing
     * @param RayTracingGeometry Bottom-level acceleration-structure to build or update
     * @param BuildDesc A structure containing information about the build
     */
    virtual void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) = 0;

    /**
     * @brief Transition the ResourceState of a batch of texture and buffer resources. Entries 
     * may freely mix textures and buffers. How each entry is handled depends on the tracking 
     * mode the resource currently has: Static entries are dropped, Manual entries are emitted 
     * verbatim from BeforeState, and Tracked entries have their before-state inferred.
     * @param TransitionDescs Transitions to perform
     */
    virtual void TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs) = 0;

    /**
     * @brief Add UnorderedAccessBarriers, which should be issued between a write and a read of a resource in UnorderedAccessState.
     * @param BarrierDescs Resources to issue barriers for, which may mix textures and buffers
     */
    virtual void UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs) = 0;

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
     * @brief Dispatches a mesh-shader pipeline with the specified thread group dimensions.
     * @param ThreadGroupCountX Number of thread groups in the X dimension.
     * @param ThreadGroupCountY Number of thread groups in the Y dimension.
     * @param ThreadGroupCountZ Number of thread groups in the Z dimension.
     */
    virtual void DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) = 0;

    /**
     * @brief Executes non-indexed draw commands read from a GPU buffer. Requires RHI::bSupportsDrawIndirect to be true.
     * @param ArgumentBuffer Buffer containing consecutive FRHIDrawIndirectParameters records.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the first record in ArgumentBuffer.
     * @param CommandCount Number of records to execute. Must be greater than zero and no greater than RHI::MaxDrawIndirectCommandCount.
     */
    virtual void DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) = 0;

    /**
     * @brief Executes non-indexed draw commands using a GPU-provided command count. Requires RHI::bSupportsDrawIndirect and RHI::bSupportsDrawIndirectCount to be true.
     * @param ArgumentBuffer Buffer containing consecutive FRHIDrawIndirectParameters records.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the first record in ArgumentBuffer.
     * @param CountBuffer Buffer containing the GPU uint32 command count.
     * @param CountBufferOffset 4-byte-aligned byte offset of the uint32 count in CountBuffer.
     * @param MaxCommandCount Maximum number of records to execute. Must be greater than zero and no greater than RHI::MaxDrawIndirectCommandCount.
     */
    virtual void DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) = 0;

    /**
     * @brief Executes indexed draw commands read from a GPU buffer. Requires RHI::bSupportsDrawIndirect to be true.
     * @param ArgumentBuffer Buffer containing consecutive FRHIDrawIndexedIndirectParameters records.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the first record in ArgumentBuffer.
     * @param CommandCount Number of records to execute. Must be greater than zero and no greater than RHI::MaxDrawIndirectCommandCount.
     */
    virtual void DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) = 0;

    /**
     * @brief Executes indexed draw commands using a GPU-provided command count. Requires RHI::bSupportsDrawIndirect and RHI::bSupportsDrawIndirectCount to be true.
     * @param ArgumentBuffer Buffer containing consecutive FRHIDrawIndexedIndirectParameters records.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the first record in ArgumentBuffer.
     * @param CountBuffer Buffer containing the GPU uint32 command count.
     * @param CountBufferOffset 4-byte-aligned byte offset of the uint32 count in CountBuffer.
     * @param MaxCommandCount Maximum number of records to execute. Must be greater than zero and no greater than RHI::MaxDrawIndirectCommandCount.
     */
    virtual void DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) = 0;

    /**
     * @brief Dispatches one compute command read from a GPU buffer. Requires RHI::bSupportsDispatchIndirect to be true.
     * @param ArgumentBuffer Buffer containing one FRHIDispatchIndirectParameters record.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the record in ArgumentBuffer.
     */
    virtual void DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) = 0;

    /**
     * @brief Executes mesh-shader dispatch commands read from a GPU buffer. Requires RHI::bSupportsDispatchMeshIndirect to be true.
     * @param ArgumentBuffer Buffer containing consecutive FRHIDispatchMeshIndirectParameters records.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the first record in ArgumentBuffer.
     * @param CommandCount Number of records to execute. Must be greater than zero and no greater than RHI::MaxDispatchMeshIndirectCommandCount.
     */
    virtual void DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) = 0;

    /**
     * @brief Executes mesh-shader dispatch commands using a GPU-provided command count. Requires RHI::bSupportsDispatchMeshIndirect and RHI::bSupportsDispatchMeshIndirectCount to be true.
     * @param ArgumentBuffer Buffer containing consecutive FRHIDispatchMeshIndirectParameters records.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the first record in ArgumentBuffer.
     * @param CountBuffer Buffer containing the GPU uint32 command count.
     * @param CountBufferOffset 4-byte-aligned byte offset of the uint32 count in CountBuffer.
     * @param MaxCommandCount Maximum number of records to execute. Must be greater than zero and no greater than RHI::MaxDispatchMeshIndirectCommandCount.
     */
    virtual void DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) = 0;

    /**
     * @brief Records the local resources for a single shader-binding-table record.
     * @param ShaderBindingTable The table to update.
     * @param RecordKind Which sub-table the record belongs to.
     * @param RecordIndex Record slot within the sub-table to update.
     * @param Bindings Array of this record's local bindings.
     * @param NumBindings Number of entries in Bindings.
     */
    virtual void SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings) = 0;

    /**
     * @brief Flushes pending record updates from the CPU shadow into the GPU shader-binding-table buffer.
     * @param ShaderBindingTable The table to update.
     */
    virtual void BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) = 0;

    /** 
     * @brief Clears all records in a shader-binding-table.
     * @param ShaderBindingTable The table to clear.
     */
    virtual void ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) = 0;

    /**
     * @brief Sets the ray tracing pipeline state used by subsequent global resource binds and DispatchRays.
     * @param PipelineState Ray tracing pipeline state.
     */
    virtual void SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState) = 0;

    /**
     * @brief Dispatches rays using a standalone shader-binding-table.
     * @param ShaderBindingTable The shader-binding-table providing the records.
     * @param Width Number of ray invocations in the X dimension.
     * @param Height Number of ray invocations in the Y dimension.
     * @param Depth Number of ray invocations in the Z dimension.
     */
    virtual void DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth) = 0;

    /**
     * @brief Dispatches rays with GPU-provided SBT ranges and dimensions. Requires RHI::bSupportsDispatchRaysIndirect to be true.
     * @param ShaderBindingTable The shader-binding-table providing the records.
     * @param ArgumentBuffer Buffer containing one FRHIDispatchRaysIndirectParameters record.
     * @param ArgumentBufferOffset 4-byte-aligned byte offset of the record in ArgumentBuffer.
     */
    virtual void DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) = 0;

    /** 
     * @brief Builds (or updates) an opacity micromap. Requires RHI::bSupportsOpacityMicromap.
     * @param OpacityMicromap The micromap to build or update.
     * @param BuildDesc Description of the build operation.
     */
    virtual void BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc) = 0;

    /**
     * @brief Executes a batch of indirect acceleration-structure operations.
     * @param Operations Array of operation descriptions.
     * @param NumOperations Number of operations in the array.
     */
    virtual void ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations) = 0;

    /**
     * @brief Emits post-build info for acceleration structures.
     * @param DstBuffer Buffer receiving the post-build info.
     * @param DstOffset Byte offset into DstBuffer.
     * @param InfoType Which post-build info to emit.
     * @param Sources Acceleration structures to query.
     * @param NumSources Number of source acceleration structures.
     */
    virtual void WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources) = 0;

    /** 
     * @brief Copies an acceleration structure with a given mode.
     * @param Destination The destination acceleration structure.
     * @param Source The source acceleration structure.
     * @param CopyMode The mode for the copy operation.
     */
    virtual void CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode) = 0;

    /**
     * @brief Compacts an acceleration structure in place. Allocates a result buffer of the compacted 
     * size, performs a Compact copy into it, then swaps it into the structure so the structure keeps
     * its identity but any TLAS that references it need to be rebuilt.
     * @param AccelerationStructure The acceleration structure to compact.
     * @param CompactedSizeInBytes The size of the compacted acceleration structure in bytes
     */
    virtual void CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes) = 0;

    /**
     * @brief Serializes an acceleration structure into a (GPU-visible) buffer.
     * @param Source The acceleration structure to serialize.
     * @param DstBuffer The destination buffer to write the serialized data into.
     * @param DstOffset The byte offset into the destination buffer to start writing.
     */
    virtual void SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset) = 0;

    /**
     * @brief Deserializes a previously serialized acceleration structure from a buffer into an acceleration structure object.
     * @param Destination The destination acceleration structure.
     * @param SourceBuffer The source buffer containing the serialized data.
     * @param SourceOffset The byte offset into the source buffer to start reading.
     */
    virtual void DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset) = 0;

    /**
     * @brief Presents the swap-chain, swapping the back buffer to the screen.
     * @param SwapChain The swap-chain to present.
     * @param bVerticalSync Whether to use vertical synchronization.
     */
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) = 0;

    /**
     * @brief Resize the swap-chain and / or change its back-buffer format and color space.
     * @param SwapChain The swap-chain to mutate.
     * @param Width New width, or 0 to keep the current width.
     * @param Height New height, or 0 to keep the current height.
     * @param Format New back-buffer format, or EFormat::Unknown to keep the current format.
     * @param ColorSpace New color space, or EColorSpace::Unknown to keep the current color space.
     */
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace) = 0;

    /**
     * @brief Submits mastering-display metadata for the swap-chain.
     * @param SwapChain The swap-chain to mutate.
     * @param Metadata The metadata to submit, or an invalid metadata to clear it.
     */
    virtual void SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata) = 0;

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
     * @param Name Name displayed for the event region in GPU profiling tools.
     */
    virtual void PushEvent(const StringView& Name) = 0;

    /**
     * @brief Ends the current GPU event region
     */
    virtual void PopEvent() = 0;

    /**
     * @brief Returns the backend-native command-list or command-buffer handle.
     * @return D3D12: ID3D12GraphicsCommandList*. Vulkan: VkCommandBuffer. Metal: nullptr. Null: nullptr.
     */
    virtual void* GetRHINativeCommandList() = 0;
};
