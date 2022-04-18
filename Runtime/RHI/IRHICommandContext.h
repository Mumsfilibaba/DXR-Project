#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIShader.h"
#include "RHIPipeline.h"

#define RHI_SHADER_LOCAL_BINDING_COUNT (4)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCopyBufferDesc

struct SCopyBufferDesc
{
    /**
     * @brief: Default Constructor
     */
    SCopyBufferDesc()
        : SourceOffset(0)
        , DestinationOffset(0)
        , Size(0)
    { }

    /**
     * @brief: Constructor that fills in the members
     * 
     * @param InSourceOffset: Offset in the source buffer
     * @param InDestinationOffset: Offset in the destination buffer
     * @param InSize: Size to copy
     */
    SCopyBufferDesc(uint64 InSourceOffset, uint32 InDestinationOffset, uint32 InSize)
        : SourceOffset(InSourceOffset)
        , DestinationOffset(InDestinationOffset)
        , Size(InSize)
    { }

    bool operator==(const SCopyBufferDesc& RHS) const
    {
        return (SourceOffset == RHS.SourceOffset) && (DestinationOffset == RHS.DestinationOffset) && (Size == RHS.Size);
    }

    bool operator!=(const SCopyBufferDesc& RHS) const
    {
        return !(*this == RHS);
    }

    uint64 SourceOffset;
    uint32 DestinationOffset;
    uint32 Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCopyTextureSubresourceInfo

struct SCopyTextureSubresourceInfo
{
    SCopyTextureSubresourceInfo()
        : SubresourceIndex(0)
        , x(0)
        , y(0)
        , z(0)
    { }

    SCopyTextureSubresourceInfo(uint32 InX, uint32 InY, uint32 InZ, uint32 InSubresourceIndex)
        : SubresourceIndex(InSubresourceIndex)
        , x(InX)
        , y(InY)
        , z(InZ)
    { }

    bool operator==(const SCopyTextureSubresourceInfo& RHS) const
    {
        return (SubresourceIndex == RHS.SubresourceIndex) && (x == RHS.x) && (y == RHS.y) && (z == RHS.z);
    }

    bool operator==(const SCopyTextureSubresourceInfo& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 SubresourceIndex;
    uint32 x;
    uint32 y;
    uint32 z;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureDesc

struct SCopyTextureDesc
{
    SCopyTextureDesc()
        : Source()
        , Destination()
        , Width(0)
        , Height(0)
        , Depth(0)
    { }

    bool operator==(const SCopyTextureDesc& RHS) const
    {
        return (Source      == RHS.Source)
            && (Destination == RHS.Destination)
            && (Width       == RHS.Width)
            && (Height      == RHS.Height)
            && (Depth       == RHS.Depth);
    }

    bool operator==(const SCopyTextureDesc& RHS) const
    {
        return !(*this == RHS);
    }

    SCopyTextureSubresourceInfo Source;
    SCopyTextureSubresourceInfo Destination;

    uint32 Width;
    uint32 Height;
    uint32 Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingShaderLocalBindings 

class CRHIRayTracingShaderLocalBindings
{
public:

    CRHIRayTracingShaderLocalBindings()
        : ShaderResourceViews()
        , NumShaderResourceViews(0)
        , UnorderedAccessViews()
        , NumUnorderedAccessView(0)
        , ConstantBuffers()
        , NumConstantBuffers(0)
    { }

    CRHIShaderResourceView*  ShaderResourceViews[RHI_SHADER_LOCAL_BINDING_COUNT];
    uint32                   NumShaderResourceViews;
    CRHIUnorderedAccessView* UnorderedAccessViews[RHI_SHADER_LOCAL_BINDING_COUNT];
    uint32                   NumUnorderedAccessView;
    CRHIConstantBuffer*      ConstantBuffers[RHI_SHADER_LOCAL_BINDING_COUNT];
    uint32                   NumConstantBuffers;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SBuildRayTracingGeometryDesc

struct SBuildRayTracingGeometryDesc
{
    SBuildRayTracingGeometryDesc()
        : VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , BuildType(ERayTracingStructureBuildType::Build)
    { }

    CRHIBuffer*                   VertexBuffer;
    CRHIBuffer*                   IndexBuffer; 
    ERayTracingStructureBuildType BuildType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SBuildRayTracingSceneDesc

struct SBuildRayTracingSceneDesc
{
    SBuildRayTracingSceneDesc()
        : Instances(nullptr)
        , NumInstances(0)
        , ShaderLocalBinding(nullptr)
        , NumShaderLocalBindings(0)
        , BuildType(ERayTracingStructureBuildType::Build)
    { }

    const CRHIRayTracingGeometryInstance*    Instances;
    uint32                                   NumInstances;
    const CRHIRayTracingShaderLocalBindings* ShaderLocalBinding;
    uint32                                   NumShaderLocalBindings;
    ERayTracingStructureBuildType            BuildType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext
{
public:

    /**
     * @brief: Start recording commands with this context 
     */
    virtual void StartContext() = 0;

    /** 
     * @brief: End recording commands with this context 
     */
    virtual void FinishContext() = 0;

    /**
     * @brief: Begin timestamp of a certain index
     * 
     * @param Query: Query to insert timestamp into
     * @param Index: Index in the query of the timestamp
     */
    virtual void BeginTimeStamp(CRHITimeQuery* Query, uint32 Index) = 0;
    
    /**
     * @brief: End timestamp of a certain index
     *
     * @param Query: Query to insert timestamp into
     * @param Index: Index in the query of the timestamp
     */
    virtual void EndTimeStamp(CRHITimeQuery* Query, uint32 Index) = 0;

    /**
     * @brief: Clear a texture as a RenderTarget
     *
     * @param Texture: Texture to clear
     * @param ClearColor: Array of float containing the ClearColor
     */
    virtual void ClearRenderTargetTexture(CRHITexture* Texture, const float ClearColor[4]) = 0;
    
    /**
     * @brief: Clear a RenderTarget
     *
     * @param RenderTargetView: RenderTargetView to clear
     * @param ClearColor: Array of float containing the ClearColor
     */
    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const float ClearColor[4]) = 0;

    /**
     * @brief: Clear a texture as a DepthStencil
     *
     * @param Texture: Texture to clear
     * @param Depth: Value to clear the depth part of the texture to
     * @param Stencil: Value to clear the stencil part of the texture to
     */
    virtual void ClearDepthStencilTexture(CRHITexture* Texture, const float Depth, uint8 Stencil) = 0;

    /**
     * @brief: Clear a DepthStencilView
     *
     * @param DepthStencilView: DepthStencilView to clear
     * @param Depth: Value to clear the depth part of the DepthStencilView to
     * @param Stencil: Value to clear the stencil part of the DepthStencilView to
     */
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) = 0;
    
    /**
     * @brief: Clear a texture with float values
     * 
     * @param Texture: Texture to clear
     * @param ClearColor: Float-values to clear the texture to 
     */
    virtual void ClearUnorderedAccessTextureFloat(CRHITexture* Texture, const float ClearColor[4]) = 0;

    /**
     * @brief: Clear a UnorderedAccessView with float values
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Float-values to clear the texture to
     */
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const float ClearColor[4]) = 0;

    /**
     * @brief: Clear a texture with uint values
     *
     * @param Texture: Texture to clear
     * @param ClearColor: Uint-values to clear the texture to
     */
    virtual void ClearUnorderedAccessTextureUint(CRHITexture* Texture, const uint32 ClearColor[4]) = 0;

    /**
     * @brief: Clear a UnorderedAccessView with uint values
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Uint-values to clear the texture to
     */
    virtual void ClearUnorderedAccessViewUint(CRHIUnorderedAccessView* UnorderedAccessView, const uint32 ClearColor[4]) = 0;

    /**
     * @brief: Set the shading rate for the entire RenderTarget
     * 
     * @param ShadingRate: ShadingRate to set the 
     */
    virtual void SetShadingRate(EShadingRate ShadingRate) = 0;

    /**
     * @brief: Set the shading rate texture
     * 
     * @param ShadingTexture: Texture containing shading-rate information
     */
    virtual void SetShadingRateTexture(CRHITexture2D* ShadingTexture) = 0;

    /**
     * @brief: Begin a RenderPass
     * 
     * @param RenderTargetViews: Array of RenderTargetViews use for the RenderPass
     * @param NumRenderTargetViews: Number of RenderTargetViews in the Array
     * @param DepthStencilView: DepthStencilView to set
     */
    virtual void BeginRenderPass(CRHIRenderTargetView* const* RenderTargetViews, uint32 NumRenderTargetViews, CRHIDepthStencilView* DepthStencilView) = 0;

    /**
     * @brief: End the current RenderPass
     */
    virtual void EndRenderPass() = 0;

    /**
     * @brief: Set the current Viewport
     * 
     * @param Width: Width of the Viewport
     * @param Height: Height of the Viewport
     * @param MinDepth: Minimum depth of the Viewport
     * @param MaxDepth: Maximum depth of the Viewport
     * @param x: x-position of the Viewport
     * @param y: y-position of the Viewport
     */
    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) = 0;

    /**
     * @brief: Set the current Scissor-Rect
     *
     * @param Width: Width of the Scissor-Rect
     * @param Height: Height of the Scissor-Rect
     * @param x: x-position of the Viewport
     * @param y: y-position of the Viewport
     */
    virtual void SetScissorRect(float Width, float Height, float x, float y) = 0;

    /**
     * @brief: Set the current blend-color
     * 
     * @param Color: New color to use as blend-color
     */
    virtual void SetBlendFactor(const CFloatColor& Color) = 0;

    /**
     * @brief: Set VertexBuffers to the Input-Assembler
     * 
     * @param VertexBuffers: Array of VertexBuffers to set
     * @param NumVertexBuffers: Number of VertexBuffers in the VertexBuffers-Array
     * @param StartBufferSlot: The slot to start set VertexBuffers at
     */
    virtual void SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 NumVertexBuffers, uint32 StartBufferSlot) = 0;

    /**
     * @brief: Set the IndexBuffer to the Input-Assembler
     * 
     * @param IndexBuffer: IndexBuffer to set
     * @param IndexFormat: Format of each index
     */
    virtual void SetIndexBuffer(CRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) = 0;

    /**
     * @brief: The primitive topology of the geometry to render
     * 
     * @param PrimitveTopologyType: Type of primitive topology
     */
    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) = 0;

    /**
     * Set the GraphicsPipelineState
     * 
     * @param PipelineState: The PipelineState to use for upcoming Draw-calls
     */
    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) = 0;

    /**
     * Set the ComputePipelineState
     *
     * @param PipelineState: The PipelineState to use for upcoming Dispatch-calls
     */
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) = 0;

    /**
     * @brief: Set shader constants
     * 
     * @param Shader: Shader to bind the constants to
     * @param Shader32BitConstants: A pointer containing the data
     * @param Num32BitConstants: Number of 32-bit constants in Shader32BitConstants
     */
    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) = 0;

    /**
     * @brief: Set a texture as a ShaderResource
     * 
     * @param Shader: Shader to bind to the texture to
     * @param Texture: Texture to bind
     * @param ParameterIndex: Texture index in the shader to bind to
     */
    virtual void SetShaderResourceTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex) = 0;

    /**
     * @brief: Set an array of textures as ShaderResources
     *
     * @param Shader: Shader to bind to the textures to
     * @param Textures: Array of textures to bind
     * @param NumTextures: Number of textures in the array to bind
     * @param StartParameterIndex: Texture index in the shader to bind the first texture to
     */
    virtual void SetShaderResourceTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex) = 0;

    /**
     * @brief: Set a ShaderResourceView
     *
     * @param Shader: Shader to bind to the texture to
     * @param ShaderResourceView: ShaderResourceView to bind
     * @param ParameterIndex: ShaderResourceView index in the shader to bind to
     */
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) = 0;

    /**
     * @brief: Set an array of ShaderResourceViews
     *
     * @param Shader: Shader to bind to the textures to
     * @param ShaderResourceViews: Array of ShaderResourceViews to bind
     * @param NumShaderResourceViews: Number of ShaderResourceViews in the array to bind
     * @param StartParameterIndex: ShaderResourceView index in the shader to bind the first ShaderResourceView to
     */
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 StartParameterIndex) = 0;

    /**
     * @brief: Set a texture as a UnorderedAccess-resource
     *
     * @param Shader: Shader to bind to the texture to
     * @param Texture: Texture to bind
     * @param ParameterIndex: Texture index in the shader to bind to
     */
    virtual void SetUnorderedAccessTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex) = 0;

    /**
     * @brief: Set an array of textures as UnorderedAccess-resources
     *
     * @param Shader: Shader to bind to the textures to
     * @param Textures: Array of textures to bind
     * @param NumTextures: Number of textures in the array to bind
     * @param StartParameterIndex: Texture index in the shader to bind the first texture to
     */
    virtual void SetUnorderedAccessTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex) = 0;

    /**
     * @brief: Set a UnorderedAccessView
     *
     * @param Shader: Shader to bind to the texture to
     * @param UnorderedAccessView: UnorderedAccessView to bind
     * @param ParameterIndex: UnorderedAccessView index in the shader to bind to
     */
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) = 0;

    /**
     * @brief: Set an array of textures as UnorderedAccess-resources
     *
     * @param Shader: Shader to bind to the textures to
     * @param UnorderedAccessViews: Array of UnorderedAccessViews to bind
     * @param NumTextures: Number of UnorderedAccessViews in the array to bind
     * @param StartParameterIndex: UnorderedAccessView index in the shader to bind the first UnorderedAccessView to
     */
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 StartParameterIndex) = 0;

    /**
     * @breif: Bind a ConstantBuffer to a shader
     * 
     * @param Shader: Shader to bind the buffer to
     * @param ConstantBuffer: Buffer to bind
     * @param ParameterIndex: ConstantBuffer index to bind to
     */
    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    
    /**
     * @breif: Bind a ConstantBuffer to a shader
     *
     * @param Shader: Shader to bind the buffer to
     * @param ConstantBuffers: Array of Buffers to bind
     * @param NumConstantBuffers: Number of buffers in the array
     * @param StartParameterIndex: ConstantBuffer index to bind the first buffer to
     */
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 StartParameterIndex) = 0;

    /**
     * @brief: Set a SamplerState to a Shader
     * 
     * @param Shader: Shader to bind to 
     * @param SamplerState: SamplerState to bind
     * @param ParameterIndex: SamplerState index to bind the SamplerState to
     */
    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) = 0;

    /**
     * @brief: Set a SamplerState to a Shader
     *
     * @param Shader: Shader to bind to
     * @param SamplerStates: Array of SamplerStates to bind
     * @param NumSamplerStates: Number of SamplerStates in the array
     * @param StartParameterIndex: SamplerState index to bind the first SamplerState to
     */
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 StartParameterIndex) = 0;

    /**
     * @brief: Update buffer with data
     * 
     * @param Dst: Buffer to update
     * @param Offset: Offset in the buffer
     * @param Size: Size of the SourceData
     * @param SourceData: A pointer to the data that should be copied into the buffer
     */
    virtual void UpdateBuffer(CRHIBuffer* Dst, uint64 Offset, uint64 Size, const void* SourceData) = 0;

    /**
     * @brief: Update a texture2D with data
     * 
     * @param Dst: Texture to update
     * @param Width: Width of the TextureData
     * @param Height: Height of the TextureData
     * @param MipLevel: MipLevel to update
     * @param SourceData: A pointer to the data that should be copied to the Texture
     */
    virtual void UpdateTexture2D(CRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* MipData) = 0;

    /**
     * @brief: Update a texture2D with data
     *
     * @param Dst: Texture to update
     * @param Width: Width of the TextureData
     * @param Height: Height of the TextureData
     * @param ArrayIndex: ArrayIndex of the TextureData
     * @param MipLevel: MipLevel to update
     * @param SourceData: A pointer to the data that should be copied to the Texture
     */
    virtual void UpdateTexture2DArray(CRHITexture2DArray* Dst, uint32 Width, uint32 Height, uint32 MipLevel, uint32 ArrayIndex, const void* SourceData) = 0;

    /**
     * @brief: Resolve a texture
     * 
     * @param Dst: Destination texture of the resolve, must have a single sample (1x MSAA)
     * @param Src: Source texture of the resolve, must have multiple samples (2x - 16x MSAA)
     */
    virtual void ResolveTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    
    /**
     * @brief: Copy buffers
     * 
     * @param Dst: Destination buffer
     * @param Src: Source buffer
     * @param CopyDesc: Copy description
     */
    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SCopyBufferDesc& CopyDesc) = 0;

    /**
     * @brief: Copy textures that have the same parameters
     * 
     * @param Dst: Destination texture
     * @param Src: Source texture
     */
    virtual void CopyTexture(CRHITexture* Dst, CRHITexture* Src) = 0;

    /**
     * @brief: Copy a region of one texture into another
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     * @param CopyTextureDesc: Description of the texture region to copy
     */
    virtual void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SCopyTextureDesc& CopyTextureDesc) = 0;

    /**
     * @brief: Enqueue the resource for being destroyed, the resource should not be used anymore after this call
     * 
     * @param Resource: Resource to destroy
     */
    virtual void DestroyResource(CRHIResource* Resource) = 0;

    /**
     * @brief: Inform the driver that we do not care about the contents of this resource anymore, the resource should be a texture or buffer
     * 
     * @param Resource: Resource to discard
     */
    virtual void DiscardContents(CRHIResource* Resource) = 0;

    /**
     * @brief: Build the acceleration structure of a RayTracing-Geometry instance
     * 
     * @param Geometry: Geometry to build acceleration structure for
     * @param BuildDesc: Description of the acceleration structure to build
    */
    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, const SBuildRayTracingGeometryDesc& BuildDesc) = 0;

    /**
     * @brief: Build the acceleration structure of a RayTracing-Geometry instance
     * 
     * @param Geometry: Geometry to build acceleration structure for
     * @param BuildDesc: Description of the acceleration structure to build
    */
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SBuildRayTracingSceneDesc& BuildDesc) = 0;

    /**
     * @brief: Generate MipLevels for a texture
     * 
     * @param Texture: Texture to generate MipLevels for
     */
    virtual void GenerateMips(CRHITexture* Texture) = 0;

    /**
     * @brief: Transition a texture to a new state
     * 
     * @param Texture: Texture to transition
     * @param BeforeState: Previous state of the Texture
     * @param AfterState: New state of the Texture
     */
    virtual void TransitionTexture(CRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;

    /**
     * @brief: Transition a Buffer to a new state
     * 
     * @param Buffer: Buffer to transition
     * @param BeforeState: Previous state of the Buffer
     * @param AfterState: New state of the Buffer
     */
    virtual void TransitionBuffer(CRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;

    /**
     * @brief: Insert a UnorderAccess barrier to ensure that the write operations finish before continuing
     * 
     * @param Texture: Texture that needs a barrier
     */
    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) = 0;

    /**
     * @brief: Insert a UnorderAccess barrier to ensure that the write operations finish before continuing
     * 
     * @param Buffer: Buffer that needs a barrier
     */
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) = 0;

    /**
     * @brief: Make a draw-call
     * 
     * @param VertexCount: Number of vertices to render
     * @param StartVertexLocation: Vertex to start render (Index in VertexBuffer)
     */
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) = 0;

    /**
     * @brief: Make a draw-call with an IndexBuffer
     * 
     * @param IndexCount: Number of indices to render
     * @param StartIndexLocation: Index to start render (Index in IndexBuffer)
     * @param BaseVertexLocation: Vertex to start render (Index in VertexBuffer)
     */
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) = 0;

    /**
     * @brief: Make a draw-call using instances
     * 
     * @param VertexCountPerInstance: Number of vertices per instance
     * @param InstanceCount: Number of instances to render
     * @param StartVertexLocation: Vertex to start render (Index in VertexBuffer)
     * @param StartInstanceLocation: Instance to start render (Index in InstanceBuffer)
     */
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) = 0;
    
    /**
     * @brief: Make a draw-call using instances and an IndexBuffer
     * 
     * @param IndexCountPerInstance: Number of indices per instance
     * @param InstanceCount: Number of instances to render
     * @param StartIndexLocation: Index to start render (Index in IndexBuffer)
     * @param BaseVertexLocation: Vertex to start render (Index in VertexBuffer)
     * @param StartInstanceLocation: Instance to start render (Index in InstanceBuffer)
     */
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) = 0;

    /**
     * @brief: Make a compute dispatch
     * 
     * @param ComputeShader: ComputeShader to use for the Dispatch
     * @param WorkGroupsX: Number of WorkGroups on the x-axis
     * @param WorkGroupsY: Number of WorkGroups on the y-axis
     * @param WorkGroupsZ: Number of WorkGroups on the z-axis
     */
    virtual void Dispatch(CRHIComputeShader* ComputeShader, uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    /**
     * @brief: Make a Ray Tracing dispatch
     * 
     * @param Scene: RayTracingScene to trace rays into
     * @param PipelineState: PipelineState to use
     * @param Width: Width of the ray-grid
     * @param Height: Height of the ray-grid
     * @param Depth: Depth of the ray-grid
     */
    virtual void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) = 0;

    /**
     * @brief: Present the current BackBuffer
     * 
     * @param Viewport: Viewport to present
     * @param bVerticalSync: Use VerticalSync when presenting
     */
    virtual void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync) = 0;

    /**
     * @brief: Clear all the current state that is set to the CommandContext
     */
    virtual void ClearState() = 0;

    /**
     * @brief: Flush the context and ensure that the Commands are submitted to the GPU
     */
    virtual void Flush() = 0;

    /**
     * @brief: Insert a debug-marker
     */
    virtual void InsertMarker(const String& Message) = 0;

    /**
     * @brief: Begin an external capture (PIX)
     */
    virtual void BeginExternalCapture() = 0;

    /**
     * @brief: End an external capture (PIX)
     */
    virtual void EndExternalCapture() = 0;

    /**
     * @brief: Retrieve the native CommandList (D3D12 and Vulkan) 
     * 
     * @return: Returns the native CommandList 
    */
    virtual void* GetRHIHandle() const = 0;
};
