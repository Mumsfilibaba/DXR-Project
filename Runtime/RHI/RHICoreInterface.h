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

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
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

inline const CHAR* ToString(ERHIShadingRateTier Tier)
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
// FShadingRateSupport

struct FShadingRateSupport
{
    FShadingRateSupport()
        : Tier(ERHIShadingRateTier::NotSupported)
        , ShadingRateImageTileSize(0)
    { }

    FShadingRateSupport(ERHIShadingRateTier InTier, uint8 InShadingRateImageTileSize)
        : Tier(InTier)
        , ShadingRateImageTileSize(InShadingRateImageTileSize)
    { }

    bool operator==(const FShadingRateSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (ShadingRateImageTileSize == RHS.ShadingRateImageTileSize);
    }

    bool operator!=(const FShadingRateSupport& RHS) const
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

inline const CHAR* ToString(ERHIRayTracingTier Tier)
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
// FRayTracingSupport

struct FRayTracingSupport
{
    FRayTracingSupport()
        : Tier(ERHIRayTracingTier::NotSupported)
        , MaxRecursionDepth(0)
    { }

    FRayTracingSupport(ERHIRayTracingTier InTier, uint8 InMaxRecursionDepth)
        : Tier(InTier)
        , MaxRecursionDepth(InMaxRecursionDepth)
    { }

    bool operator==(const FRayTracingSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (MaxRecursionDepth == RHS.MaxRecursionDepth);
    }

    bool operator!=(const FRayTracingSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIRayTracingTier Tier;
    uint8              MaxRecursionDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIInterface

class FRHIInterface
{
protected:
    FRHIInterface(ERHIInstanceType InRHIType)
        : RHIType(InRHIType)
    { }

    virtual ~FRHIInterface() = default;

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
    virtual FRHITextureCubeArray* RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer) = 0;

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
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer) = 0;

    /**
     * @brief: Creates a VertexBuffer
     *
     * @param Initializer: State that contains information about a VertexBuffer
     * @return: Returns the newly created Buffer
     */
    virtual FRHIVertexBuffer* RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer) = 0;
    
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
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView for a Buffer
     *
     * @param Initializer: Struct containing information about the ShaderResourceView
     * @return: Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView for a Texture
     *
     * @param Initializer: Struct containing information about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new UnorderedAccessView for a Buffer
     *
     * @param Initializer: Struct containing information about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer) = 0;

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
    virtual FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Domain Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Geometry Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Mesh Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Amplification Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;
    
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
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new RasterizerState
     *
     * @param CreateInfo: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new BlendState
     *
     * @param CreateInfo: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& Initializer) = 0;

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
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer) = 0;
    
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
    virtual void RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport) const = 0;

    /**
     * @brief: Check for Shading-rate support
     *
     * @param OutSupport: Struct containing the Shading-rate support for the system and current RHI
     */
    virtual void RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const = 0;

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
    virtual FString GetAdapterDescription() const { return ""; }

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
    return GRHIInterface->RHICreateTexture2D(Initializer);
}

FORCEINLINE FRHITexture2DArray* RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
{
    return GRHIInterface->RHICreateTexture2DArray(Initializer);
}

FORCEINLINE FRHITextureCube* RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)
{
    return GRHIInterface->RHICreateTextureCube(Initializer);
}

FORCEINLINE FRHITextureCubeArray* RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer)
{
    return GRHIInterface->RHICreateTextureCubeArray(Initializer);
}

FORCEINLINE FRHITexture3D* RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)
{
    return GRHIInterface->RHICreateTexture3D(Initializer);
}

FORCEINLINE FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateSamplerState(Initializer);
}

FORCEINLINE FRHIVertexBuffer* RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
{
    return GRHIInterface->RHICreateVertexBuffer(Initializer);
}

FORCEINLINE FRHIIndexBuffer* RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
{
    return GRHIInterface->RHICreateIndexBuffer(Initializer);
}

FORCEINLINE FRHIGenericBuffer* RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
{
    return GRHIInterface->RHICreateGenericBuffer(Initializer);
}

FORCEINLINE FRHIConstantBuffer* RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
{
    return GRHIInterface->RHICreateConstantBuffer(Initializer);
}

FORCEINLINE FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    return GRHIInterface->RHICreateRayTracingScene(Initializer);
}

FORCEINLINE FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    return GRHIInterface->RHICreateRayTracingGeometry(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer)
{
    return GRHIInterface->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer)
{
    return GRHIInterface->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer)
{
    return GRHIInterface->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer)
{
    return GRHIInterface->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateComputeShader(ShaderCode);
}

FORCEINLINE FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateVertexShader(ShaderCode);
}

FORCEINLINE FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateHullShader(ShaderCode);
}

FORCEINLINE FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateDomainShader(ShaderCode);
}

FORCEINLINE FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateGeometryShader(ShaderCode);
}

FORCEINLINE FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateMeshShader(ShaderCode);
}

FORCEINLINE FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateAmplificationShader(ShaderCode);
}

FORCEINLINE FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreatePixelShader(ShaderCode);
}

FORCEINLINE FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateRayGenShader(ShaderCode);
}

FORCEINLINE FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->RHICreateRayMissShader(ShaderCode);
}

FORCEINLINE FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return GRHIInterface->RHICreateVertexInputLayout(Initializer);
}

FORCEINLINE FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateDepthStencilState(Initializer);
}

FORCEINLINE FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateRasterizerState(Initializer);
}

FORCEINLINE FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateBlendState(Initializer);
}

FORCEINLINE FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateGraphicsPipelineState(Initializer);
}

FORCEINLINE FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateComputePipelineState(Initializer);
}

FORCEINLINE FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    return GRHIInterface->RHICreateRayTracingPipelineState(Initializer);
}

FORCEINLINE class FRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInterface->RHICreateTimestampQuery();
}

FORCEINLINE class FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer)
{
    return GRHIInterface->RHICreateViewport(Initializer);
}

FORCEINLINE bool RHIQueryUAVFormatSupport(EFormat Format)
{
    return GRHIInterface->RHIQueryUAVFormatSupport(Format);
}

FORCEINLINE class IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GRHIInterface->RHIGetDefaultCommandContext();
}

FORCEINLINE FString RHIGetAdapterName()
{
    return GRHIInterface->GetAdapterDescription();
}

FORCEINLINE void RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport)
{
    GRHIInterface->RHIQueryShadingRateSupport(OutSupport);
}

FORCEINLINE void RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport)
{
    GRHIInterface->RHIQueryRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    FRayTracingSupport Support;
    RHIQueryRayTracingSupport(Support);

    return false;// (Support.Tier != ERHIRayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    FShadingRateSupport Support;
    RHIQueryShadingRateSupport(Support);

    return (Support.Tier != ERHIShadingRateTier::NotSupported);
}

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif