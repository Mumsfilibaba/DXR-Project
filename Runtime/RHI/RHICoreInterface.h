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

class FRHIRayTracingGeometry;
class FRHIRayTracingScene;

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
    virtual FRHITexture2D* RHICreateTexture2D(const FRHITexture2DInitializer& Initializer) = 0;

    /**
     * @brief: Creates a Texture2DArray
     *
     * @param Initializer: Struct with information about the Texture2DArray
     * @return: Returns the newly created texture
     */
    virtual FRHITexture2DArray* RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer) = 0;

    /**
     * @brief: Creates a TextureCube
     *
     * @param Initializer: Struct with information about the TextureCube
     * @return: Returns the newly created texture
     */
    virtual FRHITextureCube* RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer) = 0;

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
    virtual FRHITexture3D* RHICreateTexture3D(const FRHITexture3DInitializer& Initializer) = 0;

    /**
     * @brief: Create a SamplerState
     * 
     * @param Initializer: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual FRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) = 0;

    /**
     * @brief: Creates a VertexBuffer
     *
     * @param Initializer: State that contains information about a VertexBuffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIVertexBuffer* RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer) = 0;
    
    /**
     * @brief: Creates a IndexBuffer
     *
     * @param Initializer: State that contains information about a IndexBuffer
     * @return: Returns the newly created Buffer
     */
    virtual FRHIIndexBuffer* RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer) = 0;
    
    /**
     * @brief: Creates a GenericBuffer
     *
     * @param Initializer: State that contains information about a GenericBuffer
     * @return: Returns the newly created Buffer
     */
    virtual FRHIGenericBuffer* RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer) = 0;

    /**
     * @brief: Creates a ConstantBuffer
     *
     * @param Initializer: State that contains information about a ConstantBuffer
     * @return: Returns the newly created Buffer
     */
    virtual FRHIConstantBuffer* RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new Ray Tracing Scene
     * 
     * @param Initializer: Struct containing information about the Ray Tracing Scene
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new Ray tracing geometry
     *
     * @param Initializer: Struct containing information about the Ray Tracing Geometry
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView for a Texture
     * 
     * @param Initializer: Struct containing information about the ShaderResourceView
     * @return: Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView for a Buffer
     *
     * @param Initializer: Struct containing information about the ShaderResourceView
     * @return: Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView for a Texture
     *
     * @param Initializer: Struct containing information about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new UnorderedAccessView for a Buffer
     *
     * @param Initializer: Struct containing information about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer) = 0;

    /**
     * @brief: Creates a new Compute Shader
     * 
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Creates a new Vertex Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) = 0;
    
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
    virtual FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Creates a new Ray-Generation Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray Any-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray-Closest-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray-Miss Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Create a new DepthStencilState
     * 
     * @param CreateInfo: Info about a DepthStencilState
     * @return: Returns the newly created DepthStencilState
     */
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new RasterizerState
     *
     * @param CreateInfo: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual FRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new BlendState
     *
     * @param CreateInfo: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual FRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new InputLayoutState
     *
     * @param CreateInfo: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer) = 0;

    /**
     * @brief: Create a Graphics PipelineState
     * 
     * @param CreateInfo: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a Compute PipelineState
     *
     * @param CreateInfo: Info about the Compute PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a Ray-Tracing PipelineState
     *
     * @param CreateInfo: Info about the Ray-Tracing PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new Timestamp Query
     * 
     * @return: Returns the newly created Timestamp Query
     */
    virtual FRHITimestampQuery* RHICreateTimestampQuery() = 0;

    /**
     * @brief: Create a new Viewport
     * 
     * @param Initializer: Structure containing the information for the Viewport
     * @return: Returns the newly created viewport
     */
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer) = 0;

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
    virtual String GetAdapterDescription() const { return ""; }

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

FORCEINLINE FRHITexture2D* RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)
{
    return GRHIInstance->RHICreateTexture2D(Initializer);
}

FORCEINLINE FRHITexture2DArray* RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
{
    return GRHIInstance->RHICreateTexture2DArray(Initializer);
}

FORCEINLINE FRHITextureCube* RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)
{
    return GRHIInstance->RHICreateTextureCube(Initializer);
}

FORCEINLINE CRHITextureCubeArray* RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
{
    return GRHIInstance->RHICreateTextureCubeArray(Initializer);
}

FORCEINLINE FRHITexture3D* RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)
{
    return GRHIInstance->RHICreateTexture3D(Initializer);
}

FORCEINLINE FRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateSamplerState(Initializer);
}

FORCEINLINE CRHIVertexBuffer* RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateVertexBuffer(Initializer);
}

FORCEINLINE FRHIIndexBuffer* RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateIndexBuffer(Initializer);
}

FORCEINLINE FRHIGenericBuffer* RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateGenericBuffer(Initializer);
}

FORCEINLINE FRHIConstantBuffer* RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
{
    return GRHIInstance->RHICreateConstantBuffer(Initializer);
}

FORCEINLINE FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    return GRHIInstance->RHICreateRayTracingScene(Initializer);
}

FORCEINLINE FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    return GRHIInstance->RHICreateRayTracingGeometry(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer)
{
    return GRHIInstance->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer)
{
    return GRHIInstance->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer)
{
    return GRHIInstance->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer)
{
    return GRHIInstance->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateComputeShader(ShaderCode);
}

FORCEINLINE FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
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

FORCEINLINE FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreatePixelShader(ShaderCode);
}

FORCEINLINE FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayGenShader(ShaderCode);
}

FORCEINLINE FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->RHICreateRayMissShader(ShaderCode);
}

FORCEINLINE FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return GRHIInstance->RHICreateVertexInputLayout(Initializer);
}

FORCEINLINE FRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateDepthStencilState(Initializer);
}

FORCEINLINE FRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateRasterizerState(Initializer);
}

FORCEINLINE FRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateBlendState(Initializer);
}

FORCEINLINE FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateGraphicsPipelineState(Initializer);
}

FORCEINLINE FRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateComputePipelineState(Initializer);
}

FORCEINLINE FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    return GRHIInstance->RHICreateRayTracingPipelineState(Initializer);
}

FORCEINLINE class FRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInstance->RHICreateTimestampQuery();
}

FORCEINLINE class FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer)
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
    return GRHIInstance->GetAdapterDescription();
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