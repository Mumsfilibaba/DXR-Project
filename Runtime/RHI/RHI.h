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

    Null   = 1,
    D3D12  = 2,
    Vulkan = 3,
    Metal  = 4,
};

NODISCARD constexpr const CHAR* ToString(ERHIType RenderLayerApi)
{
    switch (RenderLayerApi)
    {
        case ERHIType::Null:   return "Null";
        case ERHIType::D3D12:  return "D3D12";
        case ERHIType::Vulkan: return "Vulkan";
        case ERHIType::Metal:  return "Metal";

        default: return "Unknown";
    }
}

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
    constexpr bool operator==(const FRHIVideoMemoryInfo& Other) const noexcept = default;

    /** @brief Type of memory that is queried */
    EVideoMemoryType MemoryType = EVideoMemoryType::Local;

    /** @brief The current memory-usage that is in use by the application */
    uint64 MemoryUsage = 0;

    /** @brief The current memory-budget. This is the amount of memory that the application can use */
    uint64 MemoryBudget = 0;
};

class FRHI
{
public:

    /** @brief Initializes the RHI Interface */
    static RHI_API bool Initialize();

    /** @brief Releases the RHI Interface */
    static RHI_API void Release();

    /** @return Returns the true if the RHI is initialized */
    static FORCEINLINE bool IsInitialized()
    {
        return GRHI != nullptr;
    }

    /** @return Returns the current RHI Interface */
    static FORCEINLINE FRHI* Get()
    {
        return GRHI;
    }

public:

    /** @brief Releases the RHI interface */
    virtual ~FRHI() = default;

    /** @brief Called on the RHI thread to begin a new frame. */
    virtual void BeginFrame() = 0;

    /** @brief Called on the RHI thread to end the current frame. */
    virtual void EndFrame() = 0;

    /**
     * @brief Creates a texture.
     * @param InTextureInfo Description of the RHI texture.
     * @param InInitialState Initial state of the texture.
     * @param InInitialData Initial data of the texture.
     * @return The newly created texture.
     */
    virtual FRHITexture* CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState = EResourceAccess::Common, const IRHITextureData* InInitialData = nullptr) = 0;

    /**
     * @brief Creates a buffer.
     * @param InBufferInfo Description of the RHI buffer.
     * @param InInitialState Initial state of the buffer.
     * @param InInitialData Initial data of the buffer.
     * @return The newly created buffer.
     */
    virtual FRHIBuffer* CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState = EResourceAccess::Common, const void* InInitialData = nullptr) = 0;

    /**
     * @brief Creates a sampler state.
     * @param InSamplerInfo Structure with information about the sampler state.
     * @return The newly created sampler state (may return an existing one with an increased reference count).
     */
    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) = 0;

    /**
     * @brief Creates a new viewport.
     * @param InSwapChainInfo Structure containing the information for the viewport.
     * @return The newly created viewport.
     */
    virtual FRHISwapChain* CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo) = 0;

    /**
     * @brief Creates a new ray tracing scene.
     * @param InSceneInfo Structure containing information about the ray tracing scene.
     * @return The newly created ray tracing scene.
     */
    virtual FRHIRayTracingScene* CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) = 0;

    /**
     * @brief Creates a new ray tracing geometry.
     * @param InGeometryInfo Structure containing information about the ray tracing geometry.
     * @return The newly created ray tracing geometry.
     */
    virtual FRHIRayTracingGeometry* CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) = 0;

    /**
     * @brief Creates a new shader resource view for a texture.
     * @param InInfo Structure containing information about the shader resource view.
     * @return The newly created shader resource view.
     */
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHITextureSRVInfo& InInfo) = 0;

    /**
     * @brief Creates a new shader resource view for a buffer.
     * @param InInfo Structure containing information about the shader resource view.
     * @return The newly created shader resource view.
     */
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIBufferSRVInfo& InInfo) = 0;

    /**
     * @brief Creates a new unordered access view for a texture.
     * @param InInfo Structure containing information about the unordered access view.
     * @return The newly created unordered access view.
     */
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo) = 0;

    /**
     * @brief Creates a new unordered access view for a buffer.
     * @param InInfo Structure containing information about the unordered access view.
     * @return The newly created unordered access view.
     */
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo) = 0;

    /**
     * @brief Creates a new compute shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new vertex shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new hull shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new domain shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new geometry shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new mesh shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new amplification shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new pixel shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray generation shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray any-hit shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray closest-hit shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new ray miss shader.
     * @param ShaderCode Shader bytecode used to create the shader.
     * @return The newly created shader.
     */
    virtual FRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief Creates a new depth-stencil state.
     * @param InInfo Information about the depth-stencil state.
     * @return The newly created depth-stencil state.
     */
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateInfo& InInfo) = 0;

    /**
     * @brief Creates a new rasterizer state.
     * @param InInfo Information about the rasterizer state.
     * @return The newly created rasterizer state.
     */
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateInfo& InInfo) = 0;

    /**
     * @brief Creates a new blend state.
     * @param InInitializer Information about the blend state.
     * @return The newly created blend state.
     */
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a new vertex layout.
     * @param InInitializerList Information about the vertex layout.
     * @return The newly created vertex layout.
     */
    virtual FRHIVertexLayout* CreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList) = 0;

    /**
     * @brief Creates a graphics pipeline state.
     * @param InInitializer Information about the graphics pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a compute pipeline state.
     * @param InInitializer Information about the compute pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a ray-tracing pipeline state.
     * @param InInitializer Information about the ray-tracing pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) = 0;

    /**
     * @brief Creates a new query object.
     * @param InQueryType Type of the query to create.
     * @return The newly created query object.
     */
    virtual FRHIQuery* CreateQuery(EQueryType InQueryType) = 0;

    /**
     * @brief Obtains a command context.
     * @return A command context.
     */
    virtual IRHICommandContext* ObtainCommandContext() = 0;

    /**
     * @brief Gets the result for a query.
     * @param Query Query to get the result from.
     * @param OutResult Variable to store the result.
     * @return True if the result was retrieved successfully.
     */
    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult) = 0;

    /** @brief Defers destruction of an RHI resource to the deferred deletion code. */
    virtual void EnqueueResourceDeletion(FRHIResource* Resource) = 0;

    /**
     * @brief Gets the native adapter.
     * @return The native adapter.
     */
    virtual void* GetNativeAdapter() { return nullptr; }

    /**
     * @brief Gets the native device.
     * @return The native device.
     */
    virtual void* GetNativeDevice() { return nullptr; }

    /**
     * @brief Gets the native direct (graphics) command queue.
     * @return The native direct command queue.
     */
    virtual void* GetNativeDirectCommandQueue() { return nullptr; }

    /**
     * @brief Gets the native compute command queue.
     * @return The native compute command queue.
     */
    virtual void* GetNativeComputeCommandQueue() { return nullptr; }

    /**
     * @brief Gets the native copy command queue.
     * @return The native copy command queue.
     */
    virtual void* GetNativeCopyCommandQueue() { return nullptr; }

    /**
     * @brief Checks if the current RHI supports unordered access views for the specified format.
     * @param Format Format to check.
     * @return True if unordered access views with the specified format are supported.
     */
    virtual bool QueryUAVFormatSupport(EFormat Format) const { return false; }

    /**
     * @brief Retrieves memory statistics from the RHI.
     * @param MemoryType The type of video memory to query.
     * @param OutMemoryStats Variable to store the memory statistics.
     * @return True if the statistics were retrieved successfully.
     */
    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const { return false; }

    /**
     * @brief Gets the adapter name.
     * @return A string with the adapter name.
     */
    virtual FString GetAdapterName() const { return ""; }

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

    /** @brief Global pointer for the RHI interface */
    static RHI_API FRHI* GRHI;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
