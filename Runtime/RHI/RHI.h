#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "IRHICommandContext.h"

#include "Core/Modules/ModuleManager.h"

#include "CoreApplication/Generic/GenericWindow.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHI;
class FRHIRayTracingGeometry;
class FRHIRayTracingScene;
struct IRHICommandContext;
struct FRHIRayTracingSceneDesc;
struct FRHIRayTracingGeometryDesc;


enum class ERHIType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
    Vulkan  = 3,
    Metal   = 4,
};

inline const CHAR* ToString(ERHIType RenderLayerApi)
{
    switch (RenderLayerApi)
    {
        case ERHIType::Null:   return "Null";
        case ERHIType::D3D12:  return "D3D12";
        case ERHIType::Vulkan: return "Vulkan";
        case ERHIType::Metal:  return "Metal";
        default:               return "Unknown";
    }
}


/** @brief - Global pointer for the RHIInterface */
extern RHI_API FRHI* GRHI;

/**
 * @brief - Initializes the RHI Interface and sets the global pointer
 */
RHI_API bool RHIInitialize();

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
    FRHIShadingRateSupport() = default;

    FRHIShadingRateSupport(ERHIShadingRateTier InTier, uint8 InShadingRateImageTileSize)
        : Tier(InTier)
        , ShadingRateImageTileSize(InShadingRateImageTileSize)
    {
    }

    bool operator==(const FRHIShadingRateSupport& RHS) const
    {
        return Tier == RHS.Tier && ShadingRateImageTileSize == RHS.ShadingRateImageTileSize;
    }

    bool operator!=(const FRHIShadingRateSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIShadingRateTier Tier{ERHIShadingRateTier::NotSupported};
    uint8               ShadingRateImageTileSize{0};
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
    FRHIRayTracingSupport() = default;

    FRHIRayTracingSupport(ERHIRayTracingTier InTier, uint8 InMaxRecursionDepth)
        : Tier(InTier)
        , MaxRecursionDepth(InMaxRecursionDepth)
    {
    }

    bool operator==(const FRHIRayTracingSupport& RHS) const
    {
        return Tier == RHS.Tier && MaxRecursionDepth == RHS.MaxRecursionDepth;
    }

    bool operator!=(const FRHIRayTracingSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERHIRayTracingTier Tier{ERHIRayTracingTier::NotSupported};
    uint8              MaxRecursionDepth{0};
};


struct RHI_API FRHIModule : public FModuleInterface
{
    virtual ~FRHIModule() = default;

    /**
     * @brief  - Creates the RHI Instance
     * @return - Returns the newly created RHIInstance
     */
    virtual FRHI* CreateRHI() { return nullptr; }
};


class RHI_API FRHI
{
protected:
    FRHI(ERHIType InRHIType)
        : RHIType(InRHIType)
    {
    }

public:

    virtual ~FRHI() = default;

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
     * @brief        - Create a SamplerState
     * @param InDesc - Structure with information about the SamplerState
     * @return       - Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& InDesc) = 0;

    /**
     * @brief             - Create a new Viewport
     * @param Initializer - Structure containing the information for the Viewport
     * @return            - Returns the newly created viewport
     */
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportDesc& InDesc) = 0;

    /**
     * @brief        - Create a new Ray Tracing Scene
     * @param InDesc - Struct containing information about the Ray Tracing Scene
     * @return       - Returns the newly created Ray tracing Scene
     */
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc) = 0;
    
    /**
     * @brief        - Create a new Ray tracing geometry
     * @param InDesc - Struct containing information about the Ray Tracing Geometry
     * @return       - Returns the newly created Ray tracing Geometry
     */
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) = 0;

    /**
     * @brief        - Create a new ShaderResourceView for a Texture
     * @param InDesc - Struct containing information about the ShaderResourceView
     * @return       - Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) = 0;

    /**
     * @brief        - Create a new ShaderResourceView for a Buffer
     * @param InDesc - Struct containing information about the ShaderResourceView
     * @return       - Returns the newly created ShaderResourceView
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc) = 0;
    
    /**
     * @brief        - Create a new UnorderedAccessView for a Texture
     * @param InDesc - Struct containing information about the UnorderedAccessView
     * @return       - Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc) = 0;

    /**
     * @brief        - Create a new UnorderedAccessView for a Buffer
     * @param InDesc - Struct containing information about the UnorderedAccessView
     * @return       - Returns the newly created UnorderedAccessView
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc) = 0;

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
     * @brief        - Create a new DepthStencilState
     * @param InDesc - Info about a DepthStencilState
     * @return       - Returns the newly created DepthStencilState
     */
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) = 0;

    /**
     * @brief        - Create a new RasterizerState
     * @param InDesc - Info about a RasterizerState
     * @return       - Returns the newly created RasterizerState
     */
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) = 0;

    /**
     * @brief        - Create a new BlendState
     * @param InDesc - Info about a BlendState
     * @return       - Returns the newly created BlendState
     */
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateDesc& InDesc) = 0;

    /**
     * @brief        - Create a new InputLayoutState
     * @param InDesc - Info about a InputLayoutState
     * @return       - Returns the newly created InputLayoutState
     */
    virtual FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& InDesc) = 0;

    /**
     * @brief        - Create a Graphics PipelineState
     * @param InDesc - Info about the Graphics PipelineState
     * @return       - Returns the newly created PipelineState
     */
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) = 0;
    
    /**
     * @brief        - Create a Compute PipelineState
     * @param InDesc - Info about the Compute PipelineState
     * @return       - Returns the newly created PipelineState
     */
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) = 0;
    
    /**
     * @brief        - Create a Ray-Tracing PipelineState
     * @param InDesc - Info about the Ray-Tracing PipelineState
     * @return       - Returns the newly created PipelineState
     */
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) = 0;

    /**
     * @brief  - Create a new Timestamp Query
     * @return - Returns the newly created Timestamp Query
     */
    virtual FRHITimestampQuery* RHICreateTimestampQuery() = 0;

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
    virtual FString RHIGetAdapterName() const { return ""; }

    /**
     * @brief  - retrieve the current API that is used
     * @return - Returns the current RHI's API
     */
    ERHIType GetType() const { return RHIType; }

private:
    ERHIType RHIType;
};


FORCEINLINE FRHI* GetRHI() 
{
    CHECK(GRHI != nullptr);
    return GRHI;
}

FORCEINLINE FRHITexture* RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState = EResourceAccess::Common, const IRHITextureData* InInitialData = nullptr)
{
    return GetRHI()->RHICreateTexture(InDesc, InInitialState, InInitialData);
}

FORCEINLINE FRHIBuffer* RHICreateBuffer(const FRHIBufferDesc& Desc, EResourceAccess InitialAccess = EResourceAccess::Common, const void* InitialData = nullptr)
{
    return GetRHI()->RHICreateBuffer(Desc, InitialAccess, InitialData);
}

FORCEINLINE FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& Initializer)
{
    return GetRHI()->RHICreateSamplerState(Initializer);
}

FORCEINLINE FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& Initializer)
{
    return GetRHI()->RHICreateRayTracingScene(Initializer);
}

FORCEINLINE FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& Initializer)
{
    return GetRHI()->RHICreateRayTracingGeometry(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& Initializer)
{
    return GetRHI()->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& Initializer)
{
    return GetRHI()->RHICreateShaderResourceView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& Initializer)
{
    return GetRHI()->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& Initializer)
{
    return GetRHI()->RHICreateUnorderedAccessView(Initializer);
}

FORCEINLINE FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateComputeShader(ShaderCode);
}

FORCEINLINE FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateVertexShader(ShaderCode);
}

FORCEINLINE FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateHullShader(ShaderCode);
}

FORCEINLINE FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateDomainShader(ShaderCode);
}

FORCEINLINE FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateGeometryShader(ShaderCode);
}

FORCEINLINE FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateMeshShader(ShaderCode);
}

FORCEINLINE FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateAmplificationShader(ShaderCode);
}

FORCEINLINE FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreatePixelShader(ShaderCode);
}

FORCEINLINE FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateRayGenShader(ShaderCode);
}

FORCEINLINE FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GetRHI()->RHICreateRayMissShader(ShaderCode);
}

FORCEINLINE FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& Initializer)
{
    return GetRHI()->RHICreateVertexInputLayout(Initializer);
}

FORCEINLINE FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& Initializer)
{
    return GetRHI()->RHICreateDepthStencilState(Initializer);
}

FORCEINLINE FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateDesc& Initializer)
{
    return GetRHI()->RHICreateRasterizerState(Initializer);
}

FORCEINLINE FRHIBlendState* RHICreateBlendState(const FRHIBlendStateDesc& Initializer)
{
    return GetRHI()->RHICreateBlendState(Initializer);
}

FORCEINLINE FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Initializer)
{
    return GetRHI()->RHICreateGraphicsPipelineState(Initializer);
}

FORCEINLINE FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& Initializer)
{
    return GetRHI()->RHICreateComputePipelineState(Initializer);
}

FORCEINLINE FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Initializer)
{
    return GetRHI()->RHICreateRayTracingPipelineState(Initializer);
}

FORCEINLINE class FRHITimestampQuery* RHICreateTimestampQuery()
{
    return GetRHI()->RHICreateTimestampQuery();
}

FORCEINLINE class FRHIViewport* RHICreateViewport(const FRHIViewportDesc& Initializer)
{
    return GetRHI()->RHICreateViewport(Initializer);
}

FORCEINLINE bool RHIQueryUAVFormatSupport(EFormat Format)
{
    return GetRHI()->RHIQueryUAVFormatSupport(Format);
}

FORCEINLINE IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GetRHI()->RHIObtainCommandContext();
}

FORCEINLINE FString RHIGetAdapterName()
{
    return GetRHI()->RHIGetAdapterName();
}

FORCEINLINE void RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport)
{
    GetRHI()->RHIQueryShadingRateSupport(OutSupport);
}

FORCEINLINE void RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport)
{
    GetRHI()->RHIQueryRayTracingSupport(OutSupport);
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

ENABLE_UNREFERENCED_VARIABLE_WARNING