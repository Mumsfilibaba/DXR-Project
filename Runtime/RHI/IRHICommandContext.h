#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHIViewport.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureSubresourceInfo

struct SRHICopyTextureSubresourceInfo
{
    SRHICopyTextureSubresourceInfo()
        : SubresourceIndex(0)
        , x(0)
        , y(0)
        , z(0)
    { }

    SRHICopyTextureSubresourceInfo(uint32 InX, uint32 InY, uint32 InZ, uint32 InSubresourceIndex)
        : SubresourceIndex(InSubresourceIndex)
        , x(InX)
        , y(InY)
        , z(InZ)
    { }

    bool operator==(const SRHICopyTextureSubresourceInfo& RHS) const
    {
        return (SubresourceIndex == RHS.SubresourceIndex)
            && (x                == RHS.x)
            && (y                == RHS.y)
            && (z                == RHS.z);
    }

    bool operator==(const SRHICopyTextureSubresourceInfo& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 SubresourceIndex;
    uint32 x;
    uint32 y;
    uint32 z;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureInfo

struct SRHICopyTextureInfo
{
    SRHICopyTextureInfo()
        : Source()
        , Destination()
        , Width(0)
        , Height(0)
        , Depth(0)
    { }

    bool operator==(const SRHICopyTextureInfo& RHS) const
    {
        return (Source      == RHS.Source)
            && (Destination == RHS.Destination)
            && (Width       == RHS.Width)
            && (Height      == RHS.Height)
            && (Depth       == RHS.Depth);
    }

    bool operator==(const SRHICopyTextureInfo& RHS) const
    {
        return !(*this == RHS);
    }

    SRHICopyTextureSubresourceInfo Source;
    SRHICopyTextureSubresourceInfo Destination;

    uint32 Width;
    uint32 Height;
    uint32 Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext
{
public:

    /** @brief: Start recording commands with this context */
    virtual void StartContext() = 0;

    /** @brief: End recording commands with this context */
    virtual void FinishContext() = 0;

    /**
     * @brief: Begin timestamp of a certain index
     * 
     * @param Query: Query to insert timestamp into
     * @param Index: Index in the query of the timestamp
     */
    virtual void BeginTimeStamp(CRHITimestampQuery* Query, uint32 Index) = 0;
    
    /**
     * @brief: End timestamp of a certain index
     *
     * @param Query: Query to insert timestamp into
     * @param Index: Index in the query of the timestamp
     */
    virtual void EndTimeStamp(CRHITimestampQuery* Query, uint32 Index) = 0;

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
    virtual void SetShadingRateTexture(CRHITexture* ShadingTexture) = 0;

    /**
     * @breief: Begin a RenderPass
     * 
     * @param RenderPass: Description of the RenderPass to begin
     */
    virtual void BeginRenderPass(const CRHIRenderPass& RenderPass) = 0;

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
    virtual void SetShaderResourceViews( CRHIShader* Shader
                                       , CRHIShaderResourceView* const* ShaderResourceViews
                                       , uint32 NumShaderResourceViews
                                       , uint32 StartParameterIndex) = 0;

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
    virtual void SetUnorderedAccessViews( CRHIShader* Shader
                                        , CRHIUnorderedAccessView* const* UnorderedAccessViews
                                        , uint32 NumUnorderedAccessViews
                                        , uint32 StartParameterIndex) = 0;

    /**
     * @breif: Bind a ConstantBuffer to a shader
     * 
     * @param Shader: Shader to bind the buffer to
     * @param ConstantBuffer: Buffer to bind
     * @param ParameterIndex: ConstantBuffer index to bind to
     */
    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    
    /**
     * @breif: Bind a ConstantBuffer to a shader
     *
     * @param Shader: Shader to bind the buffer to
     * @param ConstantBuffers: Array of Buffers to bind
     * @param NumConstantBuffers: Number of buffers in the array
     * @param StartParameterIndex: ConstantBuffer index to bind the first buffer to
     */
    virtual void SetConstantBuffers( CRHIShader* Shader
                                   , CRHIBuffer* const* ConstantBuffers
                                   , uint32 NumConstantBuffers
                                   , uint32 StartParameterIndex) = 0;

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
     * @param Dst
     */
    virtual void UpdateBuffer(CRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    virtual void UpdateTexture2D(CRHITexture* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) = 0;

    virtual void ResolveTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    
    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo) = 0;
    virtual void CopyConstantBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo) = 0;
    virtual void CopyTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    virtual void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SRHICopyTextureInfo& CopyTextureInfo) = 0;

    virtual void DestroyResource(class CRHIResource* Resource) = 0;
    virtual void DiscardContents(class CRHIResource* Resource) = 0;

    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate) = 0;
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const CRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) = 0;

    virtual void GenerateMips(CRHITexture* Texture) = 0;

    virtual void TransitionTexture(CRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;
    virtual void TransitionBuffer(CRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;

    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) = 0;
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) = 0;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) = 0;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) = 0;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) = 0;
    virtual void DrawIndexedInstanced( uint32 IndexCountPerInstance
                                     , uint32 InstanceCount
                                     , uint32 StartIndexLocation
                                     , uint32 BaseVertexLocation
                                     , uint32 StartInstanceLocation) = 0;

    virtual void Dispatch(CRHIComputeShader* ComputeShader, uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    virtual void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) = 0;

    virtual void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync) = 0;

    virtual void ClearState() = 0;

    virtual void Flush() = 0;

    virtual void InsertMarker(const String& Message) = 0;

    virtual void BeginExternalCapture() = 0;
    virtual void EndExternalCapture() = 0;

    virtual void* GetRHIHandle() const = 0;
};
