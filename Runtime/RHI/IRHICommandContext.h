#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHIViewport.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext : public CRefCounted
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

    virtual void SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot) = 0;
    virtual void SetIndexBuffer(CRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) = 0;

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) = 0;

    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) = 0;
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) = 0;

    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) = 0;

    virtual void SetShaderResourceTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex) = 0;
    virtual void SetShaderResourceTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex) = 0;
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) = 0;
    virtual void SetShaderResourceViews( CRHIShader* Shader
                                       , CRHIShaderResourceView* const* ShaderResourceViews
                                       , uint32 NumShaderResourceViews
                                       , uint32 StartParameterIndex) = 0;

    virtual void SetUnorderedAccessTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex) = 0;
    virtual void SetUnorderedAccessTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex) = 0;
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) = 0;
    virtual void SetUnorderedAccessViews( CRHIShader* Shader
                                        , CRHIUnorderedAccessView* const* UnorderedAccessViews
                                        , uint32 NumUnorderedAccessViews
                                        , uint32 StartParameterIndex) = 0;

    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIBuffer* ConstantBuffer, uint32 ParameterIndex) = 0;
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 StartParameterIndex) = 0;

    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) = 0;
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) = 0;

    virtual void UpdateBuffer(CRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    virtual void UpdateConstantBuffer(CRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) = 0;
    virtual void UpdateTexture2D(CRHITexture* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) = 0;

    virtual void ResolveTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    
    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo) = 0;
    virtual void CopyConstantBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo) = 0;
    virtual void CopyTexture(CRHITexture* Dst, CRHITexture* Src) = 0;
    virtual void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SRHICopyTextureInfo& CopyTextureInfo) = 0;

    virtual void DestroyResource(class CRHIResource* Resource) = 0;
    virtual void DiscardContents(class CRHIResource* Resource) = 0;

    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate) = 0;
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) = 0;

    /* Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings( CRHIRayTracingScene* RayTracingScene
                                      , CRHIRayTracingPipelineState* PipelineState
                                      , const SRHIRayTracingShaderResources* GlobalResource
                                      , const SRHIRayTracingShaderResources* RayGenLocalResources
                                      , const SRHIRayTracingShaderResources* MissLocalResources
                                      , const SRHIRayTracingShaderResources* HitGroupResources
                                      , uint32 NumHitGroupResources) = 0;

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

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    virtual void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) = 0;

    virtual void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync) = 0;

    virtual void ClearState() = 0;

    virtual void Flush() = 0;

    virtual void InsertMarker(const String& Message) = 0;

    virtual void BeginExternalCapture() = 0;
    virtual void EndExternalCapture() = 0;
};
