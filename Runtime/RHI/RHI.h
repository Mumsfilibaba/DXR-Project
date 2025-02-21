#pragma once
#include "Core/Modules/ModuleManager.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/IRHICommandContext.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHI;
class FRHIRayTracingGeometry;
class FRHIRayTracingScene;
struct IRHICommandContext;
struct FRHIRayTracingSceneInfo;
struct FRHIRayTracingGeometryInfo;

enum class ERHIType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
    Vulkan  = 3,
    Metal   = 4,
};

NODISCARD constexpr const CHAR* ToString(ERHIType RenderLayerApi)
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

/** @brief Global pointer for the RHI interface */
extern RHI_API FRHI* GRHI;

/** @brief Initializes the RHI interface and sets the global pointer */
RHI_API bool RHIInitialize();

/** @brief Releases the RHI interface */
RHI_API void RHIRelease();

struct RHI_API FRHIModule : public FModuleInterface
{
    virtual ~FRHIModule() = default;

    /**
     * @brief Creates the RHI instance
     * @return Returns the newly created RHI instance
     */
    virtual FRHI* CreateRHI() { return nullptr; }
};

enum class EVideoMemoryType
{
    Local = 1,
    NonLocal,
};

struct FRHIVideoMemoryInfo
{
    constexpr FRHIVideoMemoryInfo() noexcept = default;

    constexpr bool operator==(const FRHIVideoMemoryInfo& Other) const noexcept = default;

    /** @brief Type of memory that is queried */
    EVideoMemoryType MemoryType = EVideoMemoryType::Local;

    /** @brief The current memory-usage that is in use by the application */
    uint64 MemoryUsage = 0;

    /** @brief The current memory-budget. This is the amount of memory that the application can use */
    uint64 MemoryBudget = 0;
};

class RHI_API FRHI
{
public:

    virtual ~FRHI() = default;

    /**
     * @brief Initializes the RHI.
     * @return True if initialization is successful.
     */
    virtual bool Initialize() = 0;

    /** @brief Called on the RHI thread to begin a new frame. */
    virtual void RHIBeginFrame() = 0;

    /** @brief Called on the RHI thread to end the current frame. */
    virtual void RHIEndFrame() = 0;

    /**
     * @brief Creates a texture.
     * @param InTextureInfo Description of the RHI texture.
     * @param InInitialState Initial state of the texture.
     * @param InInitialData Initial data of the texture.
     * @return The newly created texture.
     */
    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) = 0;

    /**
     * @brief Creates a buffer.
     * @param InBufferInfo Description of the RHI buffer.
     * @param InInitialState Initial state of the buffer.
     * @param InInitialData Initial data of the buffer.
     * @return The newly created buffer.
     */
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) = 0;

    /**
     * @brief Creates a sampler state.
     * @param InSamplerInfo Structure with information about the sampler state.
     * @return The newly created sampler state (may return an existing one with an increased reference count).
     */
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) = 0;

    /**
     * @brief Creates a new viewport.
     * @param InViewportInfo Structure containing the information for the viewport.
     * @return The newly created viewport.
     */
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) = 0;

    /**
     * @brief Creates a new ray tracing scene.
     * @param InSceneInfo Structure containing information about the ray tracing scene.
     * @return The newly created ray tracing scene.
     */
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) = 0;

    /**
     * @brief Creates a new ray tracing geometry.
     * @param InGeometryInfo Structure containing information about the ray tracing geometry.
     * @return The newly created ray tracing geometry.
     */
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) = 0;

    /**
     * @brief Creates a new shader resource view for a texture.
     * @param InInfo Structure containing information about the shader resource view.
     * @return The newly created shader resource view.
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInfo& InInfo) = 0;

    /**
     * @brief Creates a new shader resource view for a buffer.
     * @param InInfo Structure containing information about the shader resource view.
     * @return The newly created shader resource view.
     */
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInfo& InInfo) = 0;

    /**
     * @brief Creates a new unordered access view for a texture.
     * @param InInfo Structure containing information about the unordered access view.
     * @return The newly created unordered access view.
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo) = 0;

    /**
     * @brief Creates a new unordered access view for a buffer.
     * @param InInfo Structure containing information about the unordered access view.
     * @return The newly created unordered access view.
     */
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo) = 0;

    /**
     * @brief Creates a new compute shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new vertex shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new hull shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new domain shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new geometry shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new mesh shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new amplification shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new pixel shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray generation shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray any-hit shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray closest-hit shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray miss shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new depth-stencil state.
     * @param InInitializer Information about the depth-stencil state.
     * @return The newly created depth-stencil state.
     */
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a new rasterizer state.
     * @param InInitializer Information about the rasterizer state.
     * @return The newly created rasterizer state.
     */
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a new blend state.
     * @param InInitializer Information about the blend state.
     * @return The newly created blend state.
     */
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a new vertex layout.
     * @param InInitializerList Information about the vertex layout.
     * @return The newly created vertex layout.
     */
    virtual FRHIVertexLayout* RHICreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList) = 0;

    /**
     * @brief Creates a graphics pipeline state.
     * @param InInitializer Information about the graphics pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a compute pipeline state.
     * @param InInitializer Information about the compute pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a ray-tracing pipeline state.
     * @param InInitializer Information about the ray-tracing pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a new query object.
     * @param InQueryType Type of the query to create.
     * @return The newly created query object.
     */
    virtual FRHIQuery* RHICreateQuery(EQueryType InQueryType) = 0;

    /**
     * @brief Obtains a command context.
     * @return A command context.
     */
    virtual IRHICommandContext* RHIObtainCommandContext() = 0;

    /**
     * @brief Gets the result for a query.
     * @param Query Query to get the result from.
     * @param OutResult Variable to store the result.
     * @return True if the result was retrieved successfully.
     */
    virtual bool RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult) = 0;

    /** @brief Defers destruction of an RHI resource to the deferred deletion code. */
    virtual void RHIEnqueueResourceDeletion(FRHIResource* Resource) = 0;

    /**
     * @brief Gets the native adapter.
     * @return The native adapter.
     */
    virtual void* RHIGetAdapter() { return nullptr; }

    /**
     * @brief Gets the native device.
     * @return The native device.
     */
    virtual void* RHIGetDevice() { return nullptr; }

    /**
     * @brief Gets the native direct (graphics) command queue.
     * @return The native direct command queue.
     */
    virtual void* RHIGetDirectCommandQueue() { return nullptr; }

    /**
     * @brief Gets the native compute command queue.
     * @return The native compute command queue.
     */
    virtual void* RHIGetComputeCommandQueue() { return nullptr; }

    /**
     * @brief Gets the native copy command queue.
     * @return The native copy command queue.
     */
    virtual void* RHIGetCopyCommandQueue() { return nullptr; }

    /**
     * @brief Checks if the current RHI supports unordered access views for the specified format.
     * @param Format Format to check.
     * @return True if unordered access views with the specified format are supported.
     */
    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const { return false; }

    /**
     * @brief Retrieves memory statistics from the RHI.
     * @param MemoryType The type of video memory to query.
     * @param OutMemoryStats Variable to store the memory statistics.
     * @return True if the statistics were retrieved successfully.
     */
    virtual bool RHIQueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const { return false; }

    /**
     * @brief Gets the adapter name.
     * @return A string with the adapter name.
     */
    virtual FString RHIGetAdapterName() const { return ""; }

    /**
     * @brief Gets the current RHI's API type.
     * @return The current RHI's API type.
     */
    ERHIType GetType() const
    {
        return RHIType;
    }

protected:
    FRHI(ERHIType InRHIType)
        : RHIType(InRHIType)
    {
    }

private:
    ERHIType RHIType;
};

FORCEINLINE FRHI* GetRHI() 
{
    CHECK(GRHI != nullptr);
    return GRHI;
}

FORCEINLINE FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState = EResourceAccess::Common, const IRHITextureData* InInitialData = nullptr)
{
    return GetRHI()->RHICreateTexture(InTextureInfo, InInitialState, InInitialData);
}

FORCEINLINE FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InitialAccess = EResourceAccess::Common, const void* InitialData = nullptr)
{
    return GetRHI()->RHICreateBuffer(InBufferInfo, InitialAccess, InitialData);
}

FORCEINLINE FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
{
    return GetRHI()->RHICreateSamplerState(InSamplerInfo);
}

FORCEINLINE FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
{
    return GetRHI()->RHICreateRayTracingScene(InSceneInfo);
}

FORCEINLINE FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
{
    return GetRHI()->RHICreateRayTracingGeometry(InGeometryInfo);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInfo& InInfo)
{
    return GetRHI()->RHICreateShaderResourceView(InInfo);
}

FORCEINLINE FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInfo& InInfo)
{
    return GetRHI()->RHICreateShaderResourceView(InInfo);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo)
{
    return GetRHI()->RHICreateUnorderedAccessView(InInfo);
}

FORCEINLINE FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo)
{
    return GetRHI()->RHICreateUnorderedAccessView(InInfo);
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

FORCEINLINE FRHIVertexLayout* RHICreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
{
    return GetRHI()->RHICreateVertexLayout(InInitializerList);
}

FORCEINLINE FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InDesc)
{
    return GetRHI()->RHICreateDepthStencilState(InDesc);
}

FORCEINLINE FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return GetRHI()->RHICreateRasterizerState(InInitializer);
}

FORCEINLINE FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return GetRHI()->RHICreateBlendState(InInitializer);
}

FORCEINLINE FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
{
    return GetRHI()->RHICreateGraphicsPipelineState(InInitializer);
}

FORCEINLINE FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer)
{
    return GetRHI()->RHICreateComputePipelineState(InInitializer);
}

FORCEINLINE FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer)
{
    return GetRHI()->RHICreateRayTracingPipelineState(InInitializer);
}

FORCEINLINE class FRHIQuery* RHICreateQuery(EQueryType InQueryType)
{
    return GetRHI()->RHICreateQuery(InQueryType);
}

FORCEINLINE class FRHIViewport* RHICreateViewport(const FRHIViewportInfo& ViewportInfo)
{
    return GetRHI()->RHICreateViewport(ViewportInfo);
}

FORCEINLINE bool RHIQueryUAVFormatSupport(EFormat Format)
{
    return GetRHI()->RHIQueryUAVFormatSupport(Format);
}

FORCEINLINE FString RHIGetAdapterName()
{
    return GetRHI()->RHIGetAdapterName();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
