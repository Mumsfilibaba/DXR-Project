#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIShader.h"
#include "RHIPipeline.h"

#include "Core/Math/IntVector2.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCopyBufferRegionInfo

struct SCopyBufferRegionInfo
{
    /**
     * @brief: Default Constructor
     */
    SCopyBufferRegionInfo()
        : SrcOffset(0)
        , SrcBuffer(nullptr)
        , DstOffset(0)
        , DstBuffer(nullptr)
        , Size(0)
    { }

    /**
     * @brief: Constructor that fills in the members
     * 
     * @param InSrcOffset: Offset in the source buffer
     * @param InDstOffset: Offset in the destination buffer
     * @param InSize: Size to copy
     */
    SCopyBufferRegionInfo( CRHIBuffer* InSrcBuffer
                         , uint32 InSrcOffset
                         , CRHIBuffer* InDstBuffer
                         , uint32 InDstOffset
                         , uint32 InSize)
        : SrcBuffer(InSrcBuffer)
        , SrcOffset(InSrcOffset)
        , DstBuffer(InDstBuffer)
        , DstOffset(InDstOffset)
        , Size(InSize)
    { }

    bool operator==(const SCopyBufferRegionInfo& RHS) const
    {
        return (SrcBuffer == RHS.SrcBuffer)
            && (SrcOffset == RHS.SrcOffset)
            && (DstBuffer == RHS.DstBuffer)
            && (DstOffset == RHS.DstOffset) 
            && (Size      == RHS.Size);
    }

    bool operator!=(const SCopyBufferRegionInfo& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBuffer* SrcBuffer;
    CRHIBuffer* DstBuffer;

    uint32      SrcOffset;
    uint32      DstOffset;

    uint32      Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCopyTexture2DRegion

struct SCopyTexture2DRegionInfo
{
    SCopyTexture2DRegionInfo()
        : SrcPosition()
        , DstPosition()
        , Width()
        , Height()
    { }

    SCopyTexture2DRegionInfo( const CInt16Vector2& InSrcPosition
                        , const CInt16Vector2& InDstPosition
                        , uint16 InWidth
                        , uint16 InHeight)
        : SrcPosition(InSrcPosition)
        , DstPosition(InDstPosition)
        , Width(InWidth)
        , Height(InHeight)
    { }

    bool operator==(const SCopyTexture2DRegionInfo& RHS) const
    {
        return (SrcPosition == RHS.SrcPosition)
            && (DstPosition == RHS.DstPosition)
            && (Width       == RHS.Width)
            && (Height      == RHS.Height);
    }

    bool operator!=(const SCopyTexture2DRegionInfo& RHS) const
    {
        return !(*this == RHS);
    }

    CInt16Vector2 SrcPosition;
    CInt16Vector2 DstPosition;
    uint16        Width;
    uint16        Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCopyTexture2DArrayRegion

struct SCopyTexture2DArrayRegion
{
    SCopyTexture2DArrayRegion()
        : SrcPosition(0, 0)
        , DstPosition(0, 0)
        , SrcArraySlice(0)
        , DstArraySlice(0)
        , NumArraySlices(0)
        , Width(0)
        , Height(0)
    { }

    SCopyTexture2DArrayRegion( const CInt16Vector2& InSrcPosition
                             , const CInt16Vector2& InDstPosition
                             , uint16 InSrcArraySlice
                             , uint16 InDstArraySlice
                             , uint16 InNumArraySlices
                             , uint16 InWidth
                             , uint16 InHeight)
        : SrcPosition(InSrcPosition)
        , DstPosition(InDstPosition)
        , SrcArraySlice(InSrcArraySlice)
        , DstArraySlice(InDstArraySlice)
        , NumArraySlices(InNumArraySlices)
        , Width(InWidth)
        , Height(InHeight)
    { }

    bool operator==(const SCopyTexture2DArrayRegion& RHS) const
    {
        return (SrcPosition    == RHS.SrcPosition)
            && (DstPosition    == RHS.DstPosition)
            && (SrcArraySlice  == RHS.SrcArraySlice)
            && (DstArraySlice  == RHS.DstArraySlice)
            && (NumArraySlices == RHS.NumArraySlices)
            && (Width          == RHS.Width)
            && (Height         == RHS.Height);
    }

    bool operator!=(const SCopyTexture2DArrayRegion& RHS) const
    {
        return !(*this == RHS);
    }

    CInt16Vector2 SrcPosition;
    CInt16Vector2 DstPosition;
    uint16        SrcArraySlice;
    uint16        DstArraySlice;
    uint16        NumArraySlices;
    uint16        Width;
    uint16        Height;
};


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingLocalShaderBindings 

class CRHIRayTracingLocalShaderBindings
{
public:

    CRHIRayTracingLocalShaderBindings()
        : ShaderResourceViews()
        , NumShaderResourceViews(0)
        , UnorderedAccessViews()
        , NumUnorderedAccessViews(0)
        , ConstantBuffers()
        , NumConstantBuffers(0)
    { }

    CRHIRayTracingLocalShaderBindings( const TStaticArray<CRHIShaderResourceView*, kRHIMaxLocalShaderBindings>& InShaderResourceViews
                                     , uint32 InNumShaderResourceViews
                                     , const TStaticArray<CRHIUnorderedAccessViewRef*, kRHIMaxLocalShaderBindings>& InUnorderedAccessViews
                                     , uint32 InNumUnorderedAccessView
                                     , const TStaticArray<CRHIConstantBuffer*, kRHIMaxLocalShaderBindings>& InConstantBuffers
                                     , uint32 InNumConstantBuffers)
        : ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessView)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
    { }

    bool operator==(const CRHIRayTracingLocalShaderBindings& RHS) const
    {
        return (ShaderResourceViews     == RHS.ShaderResourceViews)
            && (NumShaderResourceViews  == RHS.NumShaderResourceViews)
            && (UnorderedAccessViews    == RHS.UnorderedAccessViews)
            && (NumUnorderedAccessViews == RHS.NumUnorderedAccessViews)
            && (ConstantBuffers         == RHS.ConstantBuffers)
            && (NumConstantBuffers      == RHS.NumConstantBuffers);
    }

    bool operator!=(const CRHIRayTracingLocalShaderBindings& RHS) const
    {
        return !(*this == RHS);
    }

    TStaticArray<CRHIShaderResourceView*, kRHIMaxLocalShaderBindings>     ShaderResourceViews;
    uint32                                                                NumShaderResourceViews;
    TStaticArray<CRHIUnorderedAccessViewRef*, kRHIMaxLocalShaderBindings> UnorderedAccessViews;
    uint32                                                                NumUnorderedAccessViews;
    TStaticArray<CRHIConstantBuffer*, kRHIMaxLocalShaderBindings>         ConstantBuffers;
    uint32                                                                NumConstantBuffers;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SBuildRayTracingGeometryInfo

struct SBuildRayTracingGeometryInfo
{
    SBuildRayTracingGeometryInfo()
        : VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , BuildType(ERayTracingStructureBuildType::Build)
    { }

    SBuildRayTracingGeometryInfo( CRHIVertexBuffer* InVertexBuffer
                                , CRHIIndexBuffer* InIndexBuffer
                                , ERayTracingStructureBuildType InBuildType)
        : VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , BuildType(InBuildType)
    { }

    bool operator==(const SBuildRayTracingGeometryInfo& RHS) const
    {
        return (VertexBuffer == RHS.VertexBuffer)
            && (IndexBuffer  == RHS.IndexBuffer)
            && (BuildType    == RHS.BuildType);
    }

    bool operator!=(const SBuildRayTracingGeometryInfo& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIVertexBuffer*             VertexBuffer;
    CRHIIndexBuffer*              IndexBuffer; 
    ERayTracingStructureBuildType BuildType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SBuildRayTracingSceneInfo

struct SBuildRayTracingSceneInfo
{
    SBuildRayTracingSceneInfo()
        : LocalShaderBindings()
        , Instances()
        , BuildType(ERayTracingStructureBuildType::Build)
    { }

    SBuildRayTracingSceneInfo( const TArrayView<const CRHIRayTracingLocalShaderBindings>& InLocalShaderBinding
                             , const TArrayView<const CRHIRayTracingGeometryInstance>& InInstances
                             , ERayTracingStructureBuildType InBuildType)
        : LocalShaderBindings(InLocalShaderBinding)
        , Instances(InInstances)
        , BuildType(InBuildType)
    { }

    bool operator==(const SBuildRayTracingSceneInfo& RHS) const
    {
        return (LocalShaderBindings == RHS.LocalShaderBindings)
            && (Instances           == RHS.Instances)
            && (BuildType           == RHS.BuildType);
    }

    bool operator!=(const SBuildRayTracingSceneInfo& RHS) const
    {
        return !(*this == RHS);
    }

    TArrayView<const CRHIRayTracingLocalShaderBindings> LocalShaderBindings;
    TArrayView<const CRHIRayTracingGeometryInstance>    Instances;
    ERayTracingStructureBuildType                       BuildType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SSetShaderConstantsInfo

struct SSetShaderConstantsInfo
{
    SSetShaderConstantsInfo()
        : ShaderConstants()
        , NumShaderConstants()
    { }

    SSetShaderConstantsInfo( const TStaticArray<uint32, kRHIMaxShaderConstants>& InShaderConstants
                           , uint32 InNumShaderConstants)
        : ShaderConstants(InShaderConstants)
        , NumShaderConstants(InNumShaderConstants)
    { }

    SSetShaderConstantsInfo(const void* InShaderConstants, uint32 Size)
        : ShaderConstants()
        , NumShaderConstants()
    {
        const uint32 NumConstants = NMath::BytesToNum32BitConstants(Size);
        Check(NumConstants <= kRHIMaxShaderConstants);

        CMemory::Memcpy(ShaderConstants.Data(), InShaderConstants, Size);
        NumShaderConstants = NumConstants;
    }

    TStaticArray<uint32, kRHIMaxShaderConstants> ShaderConstants;
    uint32                                       NumShaderConstants;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderPassInitializer

class CRHIRenderPassInitializer
{
public:

    CRHIRenderPassInitializer()
        : RenderTargets()
        , NumRenderTargets(0)
        , DepthStencilView()
        , ShadingRateTexture(nullptr)
        , StaticShadingRate(EShadingRate::VRS_1x1)
    { }

    CRHIRenderPassInitializer( const TStaticArray<CRHIRenderTargetView, kRHIMaxRenderTargetCount>& InRenderTargets
                             , uint32 InNumRenderTargets
                             , CRHIDepthStencilView InDepthStencilView
                             , CRHITexture2D* InShadingRateTexture
                             , EShadingRate InStaticShadingRate)
        : RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , DepthStencilView(InDepthStencilView)
        , ShadingRateTexture(InShadingRateTexture)
        , StaticShadingRate(InStaticShadingRate)
    { }

    bool operator==(const CRHIRenderPassInitializer& RHS) const
    {
        return (NumRenderTargets   == RHS.NumRenderTargets)
            && (RenderTargets      == RHS.RenderTargets)
            && (DepthStencilView   == RHS.DepthStencilView)
            && (ShadingRateTexture == RHS.ShadingRateTexture)
            && (StaticShadingRate  == RHS.StaticShadingRate);
    }

    bool operator!=(const CRHIRenderPassInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TStaticArray<CRHIRenderTargetView, kRHIMaxRenderTargetCount> RenderTargets;
    uint32                                                       NumRenderTargets;

    CRHIDepthStencilView                                         DepthStencilView;

    CRHITexture2D*                                               ShadingRateTexture;
    EShadingRate                                                 StaticShadingRate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHICommandContext 

class IRHICommandContext
{
protected:
    
    IRHICommandContext()  = default;
    ~IRHICommandContext() = default;

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
    virtual void ClearRenderTargetView(const CRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor) = 0;
    
    /**
     * @brief: Clear a DepthStencilView
     *
     * @param DepthStencilView: DepthStencilView to clear
     * @param Depth: Value to clear the depth part of the DepthStencilView to
     * @param Stencil: Value to clear the stencil part of the DepthStencilView to
     */
    virtual void ClearDepthStencilView(const CRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) = 0;
    
    /**
     * @brief: Clear a texture with float values
     * 
     * @param Texture: Texture to clear
     * @param ClearColor: Float-values to clear the texture to 
     */
    virtual void ClearUnorderedAccessTextureFloat(CRHITexture* Texture, const TStaticArray<float, 4>& ClearColor) = 0;

    /**
     * @brief: Clear a UnorderedAccessView with float values
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Float-values to clear the texture to
     */
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor) = 0;

    /**
     * @brief: Clear a texture with uint values
     *
     * @param Texture: Texture to clear
     * @param ClearColor: Uint-values to clear the texture to
     */
    virtual void ClearUnorderedAccessTextureUint(CRHITexture* Texture, const TStaticArray<uint32, 4>& ClearColor) = 0;

    /**
     * @brief: Clear a UnorderedAccessView with uint values
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Uint-values to clear the texture to
     */
    virtual void ClearUnorderedAccessViewUint(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<uint32, 4>& ClearColor) = 0;

    /**
     * @brief: Begin a RenderPass
     * 
     * @param Initializer: Structure containing all information needed to start a RenderPass
     */
    virtual void BeginRenderPass(const CRHIRenderPassInitializer& Initializer) = 0;

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
    virtual void SetBlendFactor(const TStaticArray<float, 4>& Color) = 0;

    /**
     * @brief: Set VertexBuffers to the Input-Assembler
     * 
     * @param VertexBuffers: Array of VertexBuffers to set
     * @param NumVertexBuffers: Number of VertexBuffers in the VertexBuffers-Array
     * @param StartBufferSlot: The slot to start set VertexBuffers at
     */
    virtual void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 NumVertexBuffers, uint32 StartBufferSlot) = 0;

    /**
     * @brief: Set the IndexBuffer to the Input-Assembler
     * 
     * @param IndexBuffer: IndexBuffer to set
     * @param IndexFormat: Format of each index
     */
    virtual void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer) = 0;

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
     * @param ShaderConstantsInfo: Structure containing the ShaderConstants
     */
    virtual void Set32BitShaderConstants(CRHIShader* Shader, const SSetShaderConstantsInfo& ShaderConstantsInfo) = 0;

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
     * @param SrcData: A pointer to the data that should be copied into the buffer
     * @param Offset: Offset in the buffer
     * @param Size: Size of the SrcData
     */
    virtual void UpdateBuffer(CRHIBuffer* Dst, const void* SrcData, uint64 Offset, uint64 Size) = 0;

    /**
     * @brief: Update a texture2D with data
     * 
     * @param Dst: Texture to update
     * @param SrcData: A pointer to the data that should be copied to the Texture
     * @param Width: Width of the TextureData
     * @param Height: Height of the TextureData
     * @param MipLevel: MipLevel to update
     */
    virtual void UpdateTexture2D(CRHITexture2D* Dst, const void* SrcData, uint32 Width, uint32 Height, uint32 MipLevel) = 0;

    /**
     * @brief: Update a texture2D with data
     *
     * @param Dst: Texture to update
     * @param SrcData: A pointer to the data that should be copied to the Texture
     * @param Width: Width of the TextureData
     * @param Height: Height of the TextureData
     * @param MipLevel: MipLevel to update
     * @param ArrayIndex: ArrayIndex of the TextureData
     * @param NumArraySlices: Number of ArraySlices to Update
     */
    virtual void UpdateTexture2DArray( CRHITexture2DArray* Dst
                                     , const void* SrcData
                                     , uint32 Width
                                     , uint32 Height
                                     , uint32 MipLevel
                                     , uint32 ArrayIndex
                                     , uint32 NumArraySlices) = 0;

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
     */
    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src) = 0;

    /**
     * @brief: Copy buffers
     *
     * @param Dst: Destination buffer
     * @param Src: Source buffer
     * @param CopyDesc: Copy description
     */
    virtual void CopyBufferRegion(const SCopyBufferRegionInfo& CopyDesc) = 0;

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
     * @param CopyInfo: Description of the texture region to copy
     */
    virtual void CopyTexture2DRegion(CRHITexture2D* Dst, CRHITexture2D* Src, const SCopyTexture2DRegionInfo& CopyInfo) = 0;

    /**
     * @brief: Copy a region of one texture into another
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     * @param CopyInfo: Description of the texture region to copy
     */
    virtual void CopyTexture2DArrayRegion(CRHITexture2DArray* Dst, CRHITexture2DArray* Src, const SCopyTexture2DArrayRegion& CopyInfo) = 0;

    /**
     * @brief: Enqueue the resource for being destroyed, the resource should not be used anymore after this call
     * 
     * @param Resource: Resource to destroy
     */
    virtual void DestroyResource(IRHIResource* Resource) = 0;

    /**
     * @brief: Inform the driver that we do not care about the contents of this texture anymore
     * 
     * @param Resource: Resource to discard
     */
    virtual void DiscardContents(CRHITexture* Resource) = 0;

    /**
     * @brief: Build the acceleration structure of a RayTracing-Geometry instance
     * 
     * @param Geometry: Geometry to build acceleration structure for
     * @param BuildDesc: Description of the acceleration structure to build
    */
    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, const SBuildRayTracingGeometryInfo& BuildDesc) = 0;

    /**
     * @brief: Build the acceleration structure of a RayTracing-Geometry instance
     * 
     * @param Geometry: Geometry to build acceleration structure for
     * @param BuildDesc: Description of the acceleration structure to build
    */
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SBuildRayTracingSceneInfo& BuildDesc) = 0;

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
     * @param WorkGroupsX: Number of WorkGroups on the x-axis
     * @param WorkGroupsY: Number of WorkGroups on the y-axis
     * @param WorkGroupsZ: Number of WorkGroups on the z-axis
     */
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) = 0;

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
