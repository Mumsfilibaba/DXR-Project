#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"
#include "RHIModule.h"
#include "RHISamplerState.h"
#include "RHIViewport.h"
#include "RHIPipelineState.h"
#include "RHITimestampQuery.h"
#include "IRHICommandContext.h"

#include "CoreApplication/Generic/GenericWindow.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct SRHIResourceData;

class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIShadingRateTier

enum class ERHIShadingRateTier : uint8
{
    NotSupported = 0,
    Tier1        = 1,
    Tier2        = 2,
};

inline const char* ToString(ERHIShadingRateTier Tier)
{
    switch (Tier)
    {
        case ERHIShadingRateTier::NotSupported: return "NotSupported";
        case ERHIShadingRateTier::Tier1:        return "Tier1";
        case ERHIShadingRateTier::Tier2:        return "Tier2";
        default:                                return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShadingRateSupport

struct SShadingRateSupport
{
    SShadingRateSupport()
        : Tier(ERHIShadingRateTier::NotSupported)
        , ShadingRateImageTileSize(0)
    { }

    SShadingRateSupport(ERHIShadingRateTier InTier, uint8 InShadingRateImageTileSize)
        : Tier(InTier)
        , ShadingRateImageTileSize(InShadingRateImageTileSize)
    { }

    bool operator==(const SShadingRateSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (ShadingRateImageTileSize == RHS.ShadingRateImageTileSize);
    }

    bool operator!=(const SShadingRateSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIShadingRateTier Tier;
    uint8               ShadingRateImageTileSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIRayTracingTier

enum class ERHIRayTracingTier : uint8
{
    NotSupported = 0,
    Tier1        = 1,
    Tier1_1      = 2,
};

inline const char* ToString(ERHIRayTracingTier Tier)
{
    switch (Tier)
    {
        case ERHIRayTracingTier::NotSupported: return "NotSupported";
        case ERHIRayTracingTier::Tier1:        return "Tier1";
        case ERHIRayTracingTier::Tier1_1:      return "Tier1_1";
        default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayTracingSupport

struct SRayTracingSupport
{
    SRayTracingSupport()
        : Tier(ERHIRayTracingTier::NotSupported)
        , MaxRecursionDepth(0)
    { }

    SRayTracingSupport(ERHIRayTracingTier InTier, uint8 InMaxRecursionDepth)
        : Tier(InTier)
        , MaxRecursionDepth(InMaxRecursionDepth)
    { }

    bool operator==(const SRayTracingSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (MaxRecursionDepth == RHS.MaxRecursionDepth);
    }

    bool operator!=(const SRayTracingSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIRayTracingTier Tier;
    uint8              MaxRecursionDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICoreInterface

class CRHICoreInterface
{
protected:

    CRHICoreInterface(ERHIInstanceType InRHIType)
        : RHIType(InRHIType)
    { }

    virtual ~CRHICoreInterface() = default;

public:

    /**
     * @brief: Initialize the RHI instance that the engine should be using
     * 
     * @param bEnableDebug: True if the debug-layer should be enabled
     * @return: Returns true if the initialization was successful
     */
    virtual bool Initialize(bool bEnableDebug) = 0;

    /**
     * @brief: Destroys the instance
     */
    virtual void Destroy() { delete this; }

    /**
     * @brief: Creates a Texture2D
     * 
     * @param Initializer: Struct with information about the Texture2D
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2D* RHICreateTexture2D(const CRHITexture2DInitializer& Initializer) = 0;

    /**
     * @brief: Creates a Texture2DArray
     *
     * @param Initializer: Struct with information about the Texture2DArray
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DArray* RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer) = 0;

    /**
     * @brief: Creates a TextureCube
     *
     * @param Initializer: Struct with information about the TextureCube
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCube* RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer) = 0;

    /**
     * @brief: Creates a TextureCubeArray
     *
     * @param Initializer: Struct with information about the TextureCubeArray
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCubeArray* RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer) = 0;

    /**
     * @brief: Creates a Texture3D
     *
     * @param Initializer: Struct with information about the Texture3D
     * @return: Returns the newly created texture
     */
    virtual CRHITexture3D* RHICreateTexture3D(const CRHITexture3DInitializer& Initializer) = 0;

    /**
     * @brief: Create a SamplerState
     * 
     * @param Initializer: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual CRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) = 0;

    /**
     * @brief: Creates a VertexBuffer
     *
     * @param Initializer: State that contains information about a VertexBuffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIVertexBuffer* RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) = 0;
    
    /**
     * @brief: Creates a IndexBuffer
     *
     * @param Initializer: State that contains information about a IndexBuffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIIndexBuffer* RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) = 0;
    
    /**
     * @brief: Creates a GenericBuffer
     *
     * @param Initializer: State that contains information about a GenericBuffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIGenericBuffer* RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) = 0;

    /**
     * @brief: Creates a ConstantBuffer
     *
     * @param Initializer: State that contains information about a ConstantBuffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIConstantBuffer* RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new Ray Tracing Scene
     * 
     * @param Initializer: Struct containing information about the Ray Tracing Scene
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual CRHIRayTracingScene* RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new Ray tracing geometry
     *
     * @param Initializer: Struct containing information about the Ray Tracing Geometry
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual CRHIRayTracingGeometry* RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     * 
     * @param CreateInfo: Info about the ShaderResourceView
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceView* CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView
     *
     * @param CreateInfo: Info about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo) = 0;
    
    /**
     * @brief: Create a new RenderTargetView
     *
     * @param CreateInfo: Info about the RenderTargetView
     * @return: Returns the newly created RenderTargetView
     */
    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo) = 0;
    
    /**
     * @brief: Create a new DepthStencilView
     *
     * @param CreateInfo: Info about the DepthStencilView
     * @return: Returns the newly created DepthStencilView
     */
    virtual CRHIDepthStencilView* CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo) = 0;

    /**
     * @brief: Creates a new Compute Shader
     * 
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Creates a new Vertex Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Hull Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Domain Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Geometry Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Mesh Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Amplification Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Pixel Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Creates a new Ray-Generation Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray Any-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray-Closest-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray-Miss Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Create a new DepthStencilState
     * 
     * @param CreateInfo: Info about a DepthStencilState
     * @return: Returns the newly created DepthStencilState
     */
    virtual CRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new RasterizerState
     *
     * @param CreateInfo: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual CRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new BlendState
     *
     * @param CreateInfo: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual CRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new InputLayoutState
     *
     * @param CreateInfo: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual CRHIVertexInputLayout* RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer) = 0;

    /**
     * @brief: Create a Graphics PipelineState
     * 
     * @param CreateInfo: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a Compute PipelineState
     *
     * @param CreateInfo: Info about the Compute PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual CRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a Ray-Tracing PipelineState
     *
     * @param CreateInfo: Info about the Ray-Tracing PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new Timestamp Query
     * 
     * @return: Returns the newly created Timestamp Query
     */
    virtual CRHITimestampQuery* RHICreateTimestampQuery() = 0;

    /**
     * @brief: Create a new Viewport
     * 
     * @param Initializer: Structure containing the information for the Viewport
     * @return: Returns the newly created viewport
     */
    virtual CRHIViewport* RHICreateViewport(const CRHIViewportInitializer& Initializer) = 0;

    /**
     * @brief: Retrieve the default CommandContext
     * 
     * @return: Returns the default CommandContext
     */
    virtual IRHICommandContext* RHIGetDefaultCommandContext() = 0;

    /**
     * @brief: Check for Ray tracing support
     * 
     * @param OutSupport: Struct containing the Ray tracing support for the system and current RHI
     */
    virtual void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport) const = 0;

    /**
     * @brief: Check for Shading-rate support
     *
     * @param OutSupport: Struct containing the Shading-rate support for the system and current RHI
     */
    virtual void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const = 0;

    /**
     * @brief: Check if the current RHI supports UnorderedAccessViews for the specified format
     * 
     * @param Format: Format to check
     * @return: Returns true if the current RHI supports UnorderedAccessViews with the specified format
     */
    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const { return false; }

    /**
     * @brief: Retrieve the name of the Adapter
     * 
     * @return: Returns a string with the Adapter name
     */
    virtual String GetAdapterName() const { return ""; }

    /**
     * @brief: retrieve the current API that is used
     * 
     * @return: Returns the current RHI's API
     */
    ERHIInstanceType GetApi() const { return RHIType; }

protected:
    ERHIInstanceType RHIType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

FORCEINLINE CRHITexture2D* RHICreateTexture2D(const CRHITexture2DInitializer& Initializer)
{
    return GRHIInstance->RHICreateTexture2D(Initializer);
}

FORCEINLINE CRHITexture2DArray* RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
{
    return GRHIInstance->RHICreateTexture2DArray(Initializer);
}

FORCEINLINE CRHITextureCube* RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer)
{
    return GRHIInstance->RHICreateTextureCube(Initializer);
}

FORCEINLINE CRHITextureCubeArray* RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
{
    return GRHIInstance->RHICreateTextureCubeArray(Initializer);
}

FORCEINLINE CRHITexture3D* RHICreateTexture3D(const CRHITexture3DInitializer& Initializer)
{
    return GRHIInstance->RHICreateTexture3D(Initializer);
}

FORCEINLINE CRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateSamplerState(Initializer);
}

FORCEINLINE CRHIVertexBuffer* RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateVertexBuffer(Initializer);
}

FORCEINLINE CRHIIndexBuffer* RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateIndexBuffer(Initializer);
}

FORCEINLINE CRHIGenericBuffer* RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateGenericBuffer(Initializer);
}

FORCEINLINE CRHIConstantBuffer* RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateConstantBuffer(Initializer);
}

FORCEINLINE CRHIRayTracingScene* RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
{
    return GRHIInstance->RHICreateRayTracingScene(Initializer);
}

FORCEINLINE CRHIRayTracingGeometry* RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
{
    return GRHIInstance->RHICreateRayTracingGeometry(Initializer);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    return GRHIInstance->CreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    CreateInfo.Texture2D.NumMips = NumMips;
    CreateInfo.Texture2D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.NumMips = NumMips;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    CreateInfo.Texture2DArray.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCube* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.NumMips = NumMips;
    CreateInfo.TextureCube.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.NumMips = NumMips;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    CreateInfo.TextureCubeArray.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 DepthSlice, uint32 NumDepthSlices, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.NumMips = NumMips;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    CreateInfo.Texture3D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIVertexBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIIndexBuffer* Buffer, uint32 FirstIndex, uint32 NumIndices)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::IndexBuffer);
    CreateInfo.IndexBuffer.Buffer = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIGenericBuffer* Buffer, uint32 FirstElement, uint32 NumElements)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::GenericBuffer);
    CreateInfo.StructuredBuffer.Buffer = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements = NumElements;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    return GRHIInstance->CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCube* Texture, EFormat Format, uint32 Mip)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIVertexBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIIndexBuffer* Buffer, uint32 FirstIndex, uint32 NumIndices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::IndexBuffer);
    CreateInfo.IndexBuffer.Buffer = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIGenericBuffer* Buffer, uint32 FirstElement, uint32 NumElements)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::GenericBuffer);
    CreateInfo.StructuredBuffer.Buffer = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements = NumElements;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    return GRHIInstance->CreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::Texture2D);
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::Texture2DArray);
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCube* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::TextureCube);
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::TextureCubeArray);
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::Texture3D);
    CreateInfo.Format = Format;
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
{
    return GRHIInstance->CreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::Texture2D);
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::Texture2DArray);
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCube* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::TextureCube);
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::TextureCubeArray);
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateComputeShader(ShaderCode);
}

FORCEINLINE CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateVertexShader(ShaderCode);
}

FORCEINLINE CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateHullShader(ShaderCode);
}

FORCEINLINE CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateDomainShader(ShaderCode);
}

FORCEINLINE CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateGeometryShader(ShaderCode);
}

FORCEINLINE CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateMeshShader(ShaderCode);
}

FORCEINLINE CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateAmplificationShader(ShaderCode);
}

FORCEINLINE CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreatePixelShader(ShaderCode);
}

FORCEINLINE CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayGenShader(ShaderCode);
}

FORCEINLINE CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayMissShader(ShaderCode);
}

FORCEINLINE CRHIVertexInputLayout* RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer)
{
    return GRHIInstance->RHICreateVertexInputLayout(Initializer);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateDepthStencilState(Initializer);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateRasterizerState(Initializer);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateBlendState(Initializer);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateGraphicsPipelineState(Initializer);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateComputePipelineState(Initializer);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateRayTracingPipelineState(Initializer);
}

FORCEINLINE class CRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInstance->RHICreateTimestampQuery();
}

FORCEINLINE class CRHIViewport* RHICreateViewport(const CRHIViewportInitializer& Initializer)
{
    return GRHIInstance->RHICreateViewport(Initializer);
}

FORCEINLINE bool RHIQueryUAVFormatSupport(EFormat Format)
{
    return GRHIInstance->RHIQueryUAVFormatSupport(Format);
}

FORCEINLINE class IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GRHIInstance->RHIGetDefaultCommandContext();
}

FORCEINLINE String RHIGetAdapterName()
{
    return GRHIInstance->GetAdapterName();
}

FORCEINLINE void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport)
{
    GRHIInstance->RHIQueryShadingRateSupport(OutSupport);
}

FORCEINLINE void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport)
{
    GRHIInstance->RHIQueryRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    SRayTracingSupport Support;
    RHIQueryRayTracingSupport(Support);

    return false;// (Support.Tier != ERHIRayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    SShadingRateSupport Support;
    RHIQueryShadingRateSupport(Support);

    return (Support.Tier != ERHIShadingRateTier::NotSupported);
}

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif