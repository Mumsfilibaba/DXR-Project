#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"
#include "RHISamplerState.h"
#include "RHIViewport.h"
#include "RHIPipelineState.h"
#include "RHITimestampQuery.h"
#include "IRHICommandContext.h"

#include "Core/Modules/ModuleManager.h"

#include "CoreApplication/Generic/GenericWindow.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class FRHIInterface;
class FRHIRayTracingGeometry;
class FRHIRayTracingScene;
struct IRHICommandContext;

enum class ERHIInstanceType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
    Metal   = 3,
};

inline const CHAR* ToString(ERHIInstanceType RenderLayerApi)
{
    switch (RenderLayerApi)
    {
        case ERHIInstanceType::D3D12: return "D3D12";
        case ERHIInstanceType::Metal: return "Metal";
        case ERHIInstanceType::Null:  return "Null";
        default:                      return "Unknown";
    }
}

/** @brief - Global pointer for the RHIInterface */
extern RHI_API FRHIInterface* GRHIInterface;

/**
 * @brief                - Initializes the RHI Interface and sets the global pointer
 * @param InInstanceType - The RHI module that should be loaded
 */
RHI_API bool RHIInitialize(ERHIInstanceType InInstanceType);

/**
 * @brief - Releases the RHI Interface
 */
RHI_API void RHIRelease();


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

struct FRHIShadingRateSupport
{
    FRHIShadingRateSupport()
        : Tier(ERHIShadingRateTier::NotSupported)
        , ShadingRateImageTileSize(0)
    { }

    FRHIShadingRateSupport(ERHIShadingRateTier InTier, uint8 InShadingRateImageTileSize)
        : Tier(InTier)
        , ShadingRateImageTileSize(InShadingRateImageTileSize)
    { }

    bool operator==(const FRHIShadingRateSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (ShadingRateImageTileSize == RHS.ShadingRateImageTileSize);
    }

    bool operator!=(const FRHIShadingRateSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIShadingRateTier Tier;
    uint8               ShadingRateImageTileSize;
};


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

struct FRHIRayTracingSupport
{
    FRHIRayTracingSupport()
        : Tier(ERHIRayTracingTier::NotSupported)
        , MaxRecursionDepth(0)
    { }

    FRHIRayTracingSupport(ERHIRayTracingTier InTier, uint8 InMaxRecursionDepth)
        : Tier(InTier)
        , MaxRecursionDepth(InMaxRecursionDepth)
    { }

    bool operator==(const FRHIRayTracingSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (MaxRecursionDepth == RHS.MaxRecursionDepth);
    }

    bool operator!=(const FRHIRayTracingSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIRayTracingTier Tier;
    uint8              MaxRecursionDepth;
};


struct RHI_API FRHIInterfaceModule
    : public FModuleInterface
{
    virtual ~FRHIInterfaceModule() = default;

    /**
     * @brief  - Creates the RHI Instance
     * @return - Returns the newly created RHIInstance
     */
    virtual FRHIInterface* CreateInterface() { return nullptr; }
};


class RHI_API FRHIInterface
{
protected:
    FRHIInterface(ERHIInstanceType InRHIType)
        : RHIType(InRHIType)
    { }

public:

    virtual ~FRHIInterface() = default;

    /**
     * @brief  - Initialize the RHI
     * @return - Returns true if initialization is successful
     */
    virtual bool Initialize() = 0;

    /**
     * @brief                - Creates a Texture
     * @param InDesc         - Description of a RHITexture
     * @param InInitialState - Initial state of the texture
     * @param InInitialData  - Initial data of the texture
     * @return               - Returns the newly created texture
     */
    virtual FRHITexture* RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) = 0;

    /**
     * @brief                - Creates a Buffer
     * @param InDesc         - Description of a RHIBuffer
     * @param InInitialState - Initial state of the buffer
     * @param InInitialData  - Initial data of the buffer
     * @return               - Returns the newly created Buffer
     */
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData) = 0;

    /**
     * @brief             - Create a SamplerState
     * @param Initializer - Structure with information about the SamplerState
     * @return            - Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& InDesc) = 0;

    /**
     * @brief             - Create a new Ray Tracing Scene
     * @param Initializer - Struct containing information about the Ray Tracing Scene
     * @return            - Returns the newly created Ray tracing Scene
     */
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer) = 0;
    
    /**
     * @brief             - Create a new Ray tracing geometry
     * @param Initializer - Struct containing information about the Ray Tracing Geometry
     * @return            - Returns the newly created Ray tracing Geometry
     */
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer) = 0;

    /**
     * @brief             - Create a new ShaderResourceView for a Texture
     * @param Initializer - Struct containing information about the ShaderResourceView
     * @return            - Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer) = 0;

    /**
     * @brief             - Create a new ShaderResourceView for a Buffer
     * @param Initializer - Struct containing information about the ShaderResourceView
     * @return            - Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer) = 0;
    
    /**
     * @brief             - Create a new UnorderedAccessView for a Texture
     * @param Initializer - Struct containing information about the UnorderedAccessView
     * @return            - Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer) = 0;

    /**
     * @brief             - Create a new UnorderedAccessView for a Buffer
     * @param Initializer - Struct containing information about the UnorderedAccessView
     * @return            - Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer) = 0;

    /**
     * @brief            - Creates a new Compute Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief            - Creates a new Vertex Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new Hull Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new Domain Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new Geometry Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new Mesh Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new Amplification Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new Pixel Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief            - Creates a new RayGen Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new RayAnyHit Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new RayClosestHit Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief            - Creates a new RayMiss Shader
     * @param ShaderCode - Shader byte-code to create the shader of
     * @return           - Returns the newly created shader
     */
    virtual FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief             - Create a new DepthStencilState
     * @param Initializer - Info about a DepthStencilState
     * @return            - Returns the newly created DepthStencilState
     */
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer) = 0;

    /**
     * @brief             - Create a new RasterizerState
     * @param Initializer - Info about a RasterizerState
     * @return            - Returns the newly created RasterizerState
     */
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer) = 0;

    /**
     * @brief             - Create a new BlendState
     * @param Initializer - Info about a BlendState
     * @return            - Returns the newly created BlendState
     */
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& Initializer) = 0;

    /**
     * @brief             - Create a new InputLayoutState
     * @param Initializer - Info about a InputLayoutState
     * @return            - Returns the newly created InputLayoutState
     */
    virtual FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer) = 0;

    /**
     * @brief             - Create a Graphics PipelineState
     * @param Initializer - Info about the Graphics PipelineState
     * @return            - Returns the newly created PipelineState
     */
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer) = 0;
    
    /**
     * @brief             - Create a Compute PipelineState
     * @param Initializer - Info about the Compute PipelineState
     * @return            - Returns the newly created PipelineState
     */
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer) = 0;
    
    /**
     * @brief             - Create a Ray-Tracing PipelineState
     * @param Initializer - Info about the Ray-Tracing PipelineState
     * @return            - Returns the newly created PipelineState
     */
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer) = 0;

    /**
     * @brief  - Create a new Timestamp Query
     * @return - Returns the newly created Timestamp Query
     */
    virtual FRHITimestampQuery* RHICreateTimestampQuery() = 0;

    /**
     * @brief             - Create a new Viewport
     * @param Initializer - Structure containing the information for the Viewport
     * @return            - Returns the newly created viewport
     */
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer) = 0;

    /**
     * @return - Returns the a CommandContext
     */
    virtual IRHICommandContext* RHIObtainCommandContext() = 0;

    /**
     * @return - Returns the native Adapter
     */
    virtual void* RHIGetAdapter() { return nullptr; }

    /**
     * @return - Returns the native Device
     */
    virtual void* RHIGetDevice() { return nullptr; }

    /**
     * @return - Returns the native Direct (Graphics) CommandQueue
     */
    virtual void* RHIGetDirectCommandQueue() { return nullptr; }

    /**
     * @return - Returns the native Compute CommandQueue
     */
    virtual void* RHIGetComputeCommandQueue() { return nullptr; }

    /**
     * @return - Returns the native Copy CommandQueue
     */
    virtual void* RHIGetCopyCommandQueue() { return nullptr; }

    /**
     * @brief            - Check for Ray tracing support
     * @param OutSupport - Struct containing the Ray tracing support for the system and current RHI
     */
    virtual void RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const = 0;

    /**
     * @brief            - Check for Shading-rate support
     * @param OutSupport - Struct containing the Shading-rate support for the system and current RHI
     */
    virtual void RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const = 0;

    /**
     * @brief        - Check if the current RHI supports UnorderedAccessViews for the specified format
     * @param Format - Format to check
     * @return       - Returns true if the current RHI supports UnorderedAccessViews with the specified format
     */
    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const { return false; }

    /**
     * @brief  - Retrieve the name of the Adapter
     * @return - Returns a string with the Adapter name
     */
    virtual FString RHIGetAdapterDescription() const { return ""; }

    /**
     * @brief  - retrieve the current API that is used
     * @return - Returns the current RHI's API
     */
    ERHIInstanceType GetInstanceType() const { return RHIType; }

private:
    ERHIInstanceType RHIType;
};


FORCEINLINE FRHIInterface* GetRHIInterface() 
{
    CHECK(GRHIInterface != nullptr);
    return GRHIInterface;
}

FORCEINLINE FRHITexture* RHICreateTexture(
    const FRHITextureDesc& InDesc,
    EResourceAccess InInitialState = EResourceAccess::Common,
    const IRHITextureData* InInitialData = nullptr)
{
    return GetRHIInterface()->RHICreateTexture(InDesc, InInitialState, InInitialData);
}

FORCEINLINE FRHIBuffer* RHICreateBuffer(
    const FRHIBufferDesc& Desc,
    EResourceAccess InitialAccess = EResourceAccess::Common,
    const void* InitialData = nullptr)
{
    return GetRHIInterface()->RHICreateBuffer(Desc, InitialAccess, InitialData);
}

FORCEINLINE FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& Initializer)
{
    return GetRHIInterface()->RHICreateSamplerState(Initializer);
}

FORCEINLINE FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateRayTracingScene(Initializer);
}

FORCEINLINE FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateRayTracingGeometry(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateComputeShader(ShaderCode);
}

FORCEINLINE FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateVertexShader(ShaderCode);
}

FORCEINLINE FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateHullShader(ShaderCode);
}

FORCEINLINE FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateDomainShader(ShaderCode);
}

FORCEINLINE FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateGeometryShader(ShaderCode);
}

FORCEINLINE FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateMeshShader(ShaderCode);
}

FORCEINLINE FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateAmplificationShader(ShaderCode);
}

FORCEINLINE FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreatePixelShader(ShaderCode);
}

FORCEINLINE FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateRayGenShader(ShaderCode);
}

FORCEINLINE FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GetRHIInterface()->RHICreateRayMissShader(ShaderCode);
}

FORCEINLINE FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateVertexInputLayout(Initializer);
}

FORCEINLINE FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateDepthStencilState(Initializer);
}

FORCEINLINE FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateRasterizerState(Initializer);
}

FORCEINLINE FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateBlendState(Initializer);
}

FORCEINLINE FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateGraphicsPipelineState(Initializer);
}

FORCEINLINE FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateComputePipelineState(Initializer);
}

FORCEINLINE FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateRayTracingPipelineState(Initializer);
}

FORCEINLINE class FRHITimestampQuery* RHICreateTimestampQuery()
{
    return GetRHIInterface()->RHICreateTimestampQuery();
}

FORCEINLINE class FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer)
{
    return GetRHIInterface()->RHICreateViewport(Initializer);
}

FORCEINLINE bool RHIQueryUAVFormatSupport(EFormat Format)
{
    return GetRHIInterface()->RHIQueryUAVFormatSupport(Format);
}

FORCEINLINE IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GetRHIInterface()->RHIObtainCommandContext();
}

FORCEINLINE FString RHIGetAdapterName()
{
    return GetRHIInterface()->RHIGetAdapterDescription();
}

FORCEINLINE void RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport)
{
    GetRHIInterface()->RHIQueryShadingRateSupport(OutSupport);
}

FORCEINLINE void RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport)
{
    GetRHIInterface()->RHIQueryRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    FRHIRayTracingSupport Support;
    RHIQueryRayTracingSupport(Support);
    return false;// (Support.Tier != ERHIRayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    FRHIShadingRateSupport Support;
    RHIQueryShadingRateSupport(Support);
    return (Support.Tier != ERHIShadingRateTier::NotSupported);
}

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif