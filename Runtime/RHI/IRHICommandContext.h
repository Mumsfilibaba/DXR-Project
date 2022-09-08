#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"

class FRHIViewport;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRenderPassInitializer

struct FRHIRenderPassInitializer
{
    FRHIRenderPassInitializer()
        : ShadingRateTexture(nullptr)
        , DepthStencilView()
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , NumRenderTargets(0)
        , RenderTargets()
    { }

    FRHIRenderPassInitializer(
        const TStaticArray<FRHIRenderTargetView, kRHIMaxRenderTargetCount>& InRenderTargets,
        uint32 InNumRenderTargets,
        FRHIDepthStencilView InDepthStencilView,
        FRHITexture2D* InShadingRateTexture = nullptr,
        EShadingRate InStaticShadingRate = EShadingRate::VRS_1x1)
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilView(InDepthStencilView)
        , StaticShadingRate(InStaticShadingRate)
        , NumRenderTargets(InNumRenderTargets)
        , RenderTargets(InRenderTargets)
    { }

    bool operator==(const FRHIRenderPassInitializer& RHS) const
    {
        return (NumRenderTargets   == RHS.NumRenderTargets)
            && (RenderTargets      == RHS.RenderTargets)
            && (DepthStencilView   == RHS.DepthStencilView)
            && (ShadingRateTexture == RHS.ShadingRateTexture)
            && (StaticShadingRate  == RHS.StaticShadingRate);
    }

    bool operator!=(const FRHIRenderPassInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture2D*       ShadingRateTexture;

    FRHIDepthStencilView DepthStencilView;
    
    EShadingRate         StaticShadingRate;
    
    uint32               NumRenderTargets;
    TStaticArray<FRHIRenderTargetView, kRHIMaxRenderTargetCount> RenderTargets;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext
{
public:

    virtual void StartContext()  = 0;
    virtual void FinishContext() = 0;

    /**
     * @brief: Begins the timestamp with the specified index in the TimestampQuery 
     * 
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to begin
     */ 
    virtual void BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) = 0;
    
    /**
     * @brief: Ends the timestamp with the specified index in the TimestampQuery
     *
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to end
     */
    virtual void EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) = 0;

    /**
     * @brief: Clears a RenderTargetView with a specific color 
     * 
     * @param RenderTargetView: RenderTargetView to clear
     * @param ClearColor: Color to set each pixel within the RenderTargetView to
     */
    virtual void ClearRenderTargetView(
        const FRHIRenderTargetView& RenderTargetView,
        const FVector4& ClearColor) = 0;

    /**
     * @brief: Clears a DepthStencilView with a specific value
     *
     * @param DepthStencilView: DepthStencilView to clear
     * @param ClearValue: Value to set each pixel within the DepthStencilView to
     */
    virtual void ClearDepthStencilView(
        const FRHIDepthStencilView& DepthStencilView,
        const float Depth,
        const uint8 Stencil) = 0;
    
    /**
     * @brief: Clears a UnorderedAccessView with a specific value
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Value to set each pixel within the UnorderedAccessView to
     */
    virtual void ClearUnorderedAccessViewFloat(
        FRHIUnorderedAccessView* UnorderedAccessView,
        const FVector4& ClearColor) = 0;

    /**
     * @brief: Begins a new RenderPass
     *
     * @param RenderPassInitializer: Description of RenderTargets and DepthStencils to bind for drawing
     */
    virtual void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer) = 0;

    /**
     * @brief: Ends the current RenderPass
     */
    virtual void EndRenderPass() = 0;

    /**
     * @brief: Set the current viewport settings
     * 
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param MinDepth: Minimum-depth of the viewport
     * @param MaxDepth: Maximum-depth of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    virtual void SetViewport(
        float Width,
        float Height,
        float MinDepth,
        float MaxDepth,
        float x,
        float y) = 0;
    
    /**
     * @brief: Set the current scissor settings 
     * 
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    virtual void SetScissorRect(float Width, float Height, float x, float y) = 0;

    /**
     * @brief: Set the BlendFactor color 
     * 
     * @param Color: New blend-factor to use
     */
    virtual void SetBlendFactor(const FVector4& Color) = 0;

    /**
     * @brief: Set the VertexBuffers to be used
     * 
     * @param VertexBuffers: ArrayView of VertexBuffers to use
     * @param BufferSlot: Slot to start bind the array to
     */
    virtual void SetVertexBuffers(
        const TArrayView<FRHIVertexBuffer* const> InVertexBuffers,
        uint32 BufferSlot) = 0;
    
    /**
     * @brief: Set the current IndexBuffer 
     * 
     * @param IndexBuffer: IndexBuffer to use
     */
    virtual void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer) = 0;

    /**
     * @brief: Set the primitive topology 
     * 
     * @param PrimitveTopologyType: New primitive topology to use
     */
    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) = 0;

    /**
     * @brief: Sets the current graphics PipelineState 
     * 
     * @param PipelineState: New PipelineState to use
     */
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) = 0;
    
    /**
     * @brief: Sets the current compute PipelineState
     *
     * @param PipelineState: New PipelineState to use
     */
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) = 0;

    /**
     * @brief: Set shader constants
     * 
     * @param Shader: Shader to bind the constants to
     * @param Shader32BitConstants: Array of 32-bit constants
     * @param Num32bitConstants: Number o 32-bit constants (Each is 4 bytes)
     */
    virtual void Set32BitShaderConstants(
        FRHIShader* Shader,
        const void* Shader32BitConstants,
        uint32 Num32BitConstants) = 0;

    /**
     * @brief: Sets a single ShaderResourceView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object 
     * 
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceView: ShaderResourceView to bind
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    virtual void SetShaderResourceView(
        FRHIShader* Shader,
        FRHIShaderResourceView* ShaderResourceView,
        uint32 ParameterIndex) = 0;
    
    /**
     * @brief: Sets a multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceViews: ArrayView of ShaderResourceViews to bind
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    virtual void SetShaderResourceViews(
        FRHIShader* Shader,
        const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews,
        uint32 ParameterIndex) = 0;

    /**
     * @brief: Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessView: UnorderedAccessView to bind
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    virtual void SetUnorderedAccessView(
        FRHIShader* Shader,
        FRHIUnorderedAccessView* UnorderedAccessView,
        uint32 ParameterIndex) = 0;

    /**
     * @brief: Sets a multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param InUnorderedAccessViews: ArrayView of UnorderedAccessViews to bind
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    virtual void SetUnorderedAccessViews(
        FRHIShader* Shader,
        const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews,
        uint32 ParameterIndex) = 0;

    /**
     * @brief: Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffer: ConstantBuffer to bind
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    virtual void SetConstantBuffer(
        FRHIShader* Shader,
        FRHIConstantBuffer* ConstantBuffer,
        uint32 ParameterIndex) = 0;
    
    /**
     * @brief: Sets a multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffers: ArrayView of ConstantBuffers to bind
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    virtual void SetConstantBuffers(
        FRHIShader* Shader,
        const TArrayView<FRHIConstantBuffer* const> InConstantBuffers,
        uint32 ParameterIndex) = 0;

    /**
     * @brief: Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind sampler to
     * @param SamplerState: SamplerState to bind
     * @param ParameterIndex: SamplerState-index to bind to
     */
    virtual void SetSamplerState(
        FRHIShader* Shader,
        FRHISamplerState* SamplerState,
        uint32 ParameterIndex) = 0;

    /**
     * @brief: Sets a multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param SamplerStates: ArrayView of SamplerStates to bind
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    virtual void SetSamplerStates(
        FRHIShader* Shader,
        const TArrayView<FRHISamplerState* const> InSamplerStates,
        uint32 ParameterIndex) = 0;

    /**
     * @brief: Updates the contents of a Buffer
     * 
     * @param Dst: Destination buffer to update
     * @param OffsetInBytes: Offset in bytes inside the destination-buffer
     * @param SizeInBytes: Number of bytes to copy over to the buffer
     * @param SourceData: SourceData to copy to the GPU
     */
    virtual void UpdateBuffer(
        FRHIBuffer* Dst,
        uint64 OffsetInBytes,
        uint64 SizeInBytes,
        const void* SourceData) = 0;
    
    /**
     * @brief: Updates the contents of a Texture2D
     *
     * @param Dst: Destination Texture2D to update
     * @param Width: Width of the texture to update
     * @param Height: Height of the texture to update
     * @param MipLevel: MipLevel of the texture to update
     * @param SourceData: SourceData to copy to the GPU
     */
    virtual void UpdateTexture2D(
        FRHITexture2D* Dst,
        uint32 Width,
        uint32 Height,
        uint32 MipLevel,
        const void* SourceData) = 0;

    /**
     * @brief: Resolves a multi-sampled texture, must have the same sizes and compatible formats
     * 
     * @param Dst: Destination texture, must have a single sample
     * @param Src: Source texture to resolve
     */
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) = 0;
    
    /**
     * @brief: Copies the contents from one buffer to another 
     * 
     * @param Dst: Destination buffer to copy to
     * @param Src: Source buffer to copy from
     * @param CopyInfo: Information about the copy operation
     */
    virtual void CopyBuffer(
        FRHIBuffer* Dst,
        FRHIBuffer* Src,
        const FRHICopyBufferInfo& CopyInfo) = 0;
    
    /**
     * @brief: Copies the entire contents of one texture to another, which require the size and formats to be the same 
     * 
     * @param Dst: Destination texture
     * @param Src: Source texture
     */ 
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) = 0;

    /**
     * @brief: Copies contents of a texture region of one texture to another, which require the size and formats to be the same
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     * @param CopyTextureInfo: Information about the copy operation
     */
    virtual void CopyTextureRegion(
        FRHITexture* Dst,
        FRHITexture* Src,
        const FRHICopyTextureInfo& CopyTextureInfo) = 0;

    /**
     * @brief: Destroys a resource, this can be used to not having to deal with resource life time, 
     * the resource will be destroyed when the underlying command-list is completed
     * 
     * @param Resource: Resource to destroy
     */
    virtual void DestroyResource(class IRefCounted* Resource) = 0;

    /**
     * @brief: Signal the driver that the contents can be discarded
     *
     * @param Texture: Resource to discard contents of
     */
    virtual void DiscardContents(class FRHITexture* Texture) = 0;

    /**
     * @brief: Builds the Bottom-Level Acceleration-Structure for ray tracing 
     * 
     * @param Geometry: Bottom-level acceleration-structure to build or update
     * @param VertexBuffer: VertexBuffer to build Geometry of
     * @param IndexBuffer: IndexBuffer to build Geometry of
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */ 
    virtual void BuildRayTracingGeometry(
        FRHIRayTracingGeometry* Geometry,
        FRHIVertexBuffer* VertexBuffer,
        FRHIIndexBuffer* IndexBuffer,
        bool bUpdate) = 0;
    
    /**
     * @brief: Builds the Top-Level Acceleration-Structure for ray tracing
     *
     * @param Scene: Top-level acceleration-structure to build or update
     * @param Instances: Instances to build the scene of
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */
    virtual void BuildRayTracingScene(
        FRHIRayTracingScene* Scene,
        const TArrayView<const FRHIRayTracingGeometryInstance>& Instances,
        bool bUpdate) = 0;

     /** @brief: Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings(
        FRHIRayTracingScene* RayTracingScene,
        FRHIRayTracingPipelineState* PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) = 0;

    /**
     * @brief: Generate MipLevels for a texture. Works with Texture2D and TextureCubes.
     * 
     * @param Texture: Texture to generate MipLevels for
     */
    virtual void GenerateMips(FRHITexture* Texture) = 0;

    /**
     * @brief: Transition the ResourceState of a Texture resource 
     * 
     * @param Texture: Texture to transition ResourceState for
     * @param BeforeState: State that the Texture had before the transition
     * @param AfterState: State that the Texture have after the transition
     */
    virtual void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;
    
    /**
     * @brief: Transition the ResourceState of a Buffer resource
     *
     * @param Buffer: Buffer to transition ResourceState for
     * @param BeforeState: State that the Buffer had before the transition
     * @param AfterState: State that the Buffer have after the transition
     */
    virtual void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) = 0;

    /**
     * @brief: Add a UnorderedAccessBarrier for a Texture resource, which should be issued before reading of a resource in UnorderedAccessState 
     * 
     * @param Texture: Texture to issue barrier for
     */
    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) = 0;
    
    /**
     * @brief: Add a UnorderedAccessBarrier for a Buffer resource, which should be issued before reading of a resource in UnorderedAccessState
     *
     * @param Buffer: Buffer to issue barrier for
     */
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) = 0;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) = 0;
    
    virtual void DrawIndexed(
        uint32 IndexCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation) = 0;
    
    virtual void DrawInstanced(
        uint32 VertexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartVertexLocation,
        uint32 StartInstanceLocation) = 0;
    
    virtual void DrawIndexedInstanced(
        uint32 IndexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation,
        uint32 StartInstanceLocation) = 0;

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

    virtual void DispatchRays(
        FRHIRayTracingScene* Scene,
        FRHIRayTracingPipelineState* PipelineState,
        uint32 Width,
        uint32 Height,
        uint32 Depth) = 0;

    virtual void PresentViewport(FRHIViewport* Viewport, bool bVerticalSync) = 0;

    /** @brief: Clears the state of the context, clearing all bound references currently bound */
    virtual void ClearState() = 0;

    /**  @brief: Waits for all current execution on the GPU to finish  */
    virtual void Flush() = 0;

    /** @brief: Inserts a marker on the GPU timeline */
    virtual void InsertMarker(const FStringView& Message) = 0;

    /** @brief:  Begins a PIX capture event, currently only available on D3D12  */
    virtual void BeginExternalCapture() = 0;
    
    /** @brief: Ends a PIX capture event, currently only available on D3D12  */
    virtual void EndExternalCapture() = 0;

    /** @return: Returns the native CommandList */
    virtual void* GetRHIBaseCommandList() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContextManager

struct IRHICommandContextManager
{
    /** @return: Returns a new CommandContext */
    virtual IRHICommandContext* ObtainCommandContext() = 0;
    
    /** @breif: Return a CommandContext and execute it */
    virtual void FinishCommandContext(IRHICommandContext* InCommandContext) = 0;
};