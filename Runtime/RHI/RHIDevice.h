#pragma once
#include "RHI/RHITypes.h"
#include "RHI/RHIResources.h"
#include "RHI/IRHICommandContext.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIGeometryAccelerationStructure;
class FRHISceneAccelerationStructure;
class FRHIFence;
struct IRHICommandContext;
struct FRHISceneAccelerationStructureDesc;
struct FRHIGeometryAccelerationStructureDesc;

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

enum class EVideoMemoryType
{
    Local = 1,
    NonLocal,
};

struct FRHIVideoMemoryInfo
{
    /** @brief Type of memory that is queried */
    EVideoMemoryType MemoryType = EVideoMemoryType::Local;

    /** @brief The current memory-usage that is in use by the application */
    uint64 MemoryUsage = 0;

    /** @brief The current memory-budget. This is the amount of memory that the application can use */
    uint64 MemoryBudget = 0;
};

struct FRHIDevice
{
    /** @brief Releases the RHI interface */
    virtual ~FRHIDevice() = default;

    /** @brief Called on the RHI thread to begin a new frame. */
    virtual void BeginFrame() = 0;

    /** @brief Called on the RHI thread to end the current frame. */
    virtual void EndFrame() = 0;

    /**
     * @brief Creates a texture.
     * @param InTextureDesc Description of the RHI texture.
     * @param InInitialState Initial state of the texture.
     * @param InInitialData Initial data of the texture.
     * @return The newly created texture.
     */
    virtual FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState = EResourceAccess::Common, const IRHITextureData* InInitialData = nullptr) = 0;

    /**
     * @brief Creates a buffer.
     * @param InBufferDesc Description of the RHI buffer.
     * @param InInitialState Initial state of the buffer.
     * @param InInitialData Initial data of the buffer.
     * @return The newly created buffer.
     */
    virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState = EResourceAccess::Common, const void* InInitialData = nullptr) = 0;

    /**
     * @brief Creates a sampler state.
     * @param InSamplerDesc Structure with information about the sampler state.
     * @return The newly created sampler state (may return an existing one with an increased reference count).
     */
    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) = 0;

    /**
     * @brief Creates a new viewport.
     * @param InSwapChainDesc Structure containing the information for the viewport.
     * @return The newly created viewport.
     */
    virtual FRHISwapChain* CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) = 0;

    /**
     * @brief Creates a new ray tracing scene.
     * @param InSceneDesc Structure containing information about the ray tracing scene.
     * @return The newly created ray tracing scene.
     */
    virtual FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) = 0;

    /**
     * @brief Creates a new ray tracing geometry.
     * @param InGeometryDesc Structure containing information about the ray tracing geometry.
     * @return The newly created ray tracing geometry.
     */
    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) = 0;

    /**
     * @brief Creates a new shader resource view for a buffer or texture.
     * @param InResource Resource to create the view for. Buffer views require an FRHIBuffer; texture views require an FRHITexture.
     * @param InDesc Structure containing information about the shader resource view (must have an explicit EViewDimension).
     * @return The newly created shader resource view.
     */
    virtual FRHIShaderResourceView* CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc) = 0;

    /**
     * @brief Creates a new unordered access view for a buffer or texture.
     * @param InResource Resource to create the view for. Buffer views require an FRHIBuffer; texture views require an FRHITexture.
     * @param InDesc Structure containing information about the unordered access view (must have an explicit EViewDimension).
     * @return The newly created unordered access view.
     */
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc) = 0;

    /**
     * @brief Creates a new render target view for a texture.
     * @param InResource Texture to create the view for.
     * @param InDesc Structure containing information about the render target view (must have an explicit EViewDimension).
     * @return The newly created render target view.
     */
    virtual FRHIRenderTargetView* CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc) = 0;

    /**
     * @brief Creates a new depth-stencil view for a texture.
     * @param InResource Texture to create the view for.
     * @param InDesc Structure containing information about the depth-stencil view (must have an explicit EViewDimension).
     * @return The newly created depth-stencil view.
     */
    virtual FRHIDepthStencilView* CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc) = 0;

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
     * @param InDesc Information about the depth-stencil state.
     * @return The newly created depth-stencil state.
     */
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) = 0;

    /**
     * @brief Creates a new rasterizer state.
     * @param InDesc Information about the rasterizer state.
     * @return The newly created rasterizer state.
     */
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) = 0;

    /**
     * @brief Creates a new blend state.
     * @param InDesc Information about the blend state.
     * @return The newly created blend state.
     */
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateDesc& InDesc) = 0;

    /**
     * @brief Creates a new vertex layout.
     * @param InInputElements Array of InputElements.
     * @return The newly created vertex layout.
     */
    virtual FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) = 0;

    /**
     * @brief Creates a graphics pipeline state.
     * @param InDesc Information about the graphics pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) = 0;

    /**
     * @brief Creates a compute pipeline state.
     * @param InDesc Information about the compute pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) = 0;

    /**
     * @brief Creates a ray-tracing pipeline state.
     * @param InDesc Information about the ray-tracing pipeline state.
     * @return The newly created pipeline state.
     */
    virtual FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) = 0;

    /**
     * @brief Creates a new query object.
     * @param InQueryType Type of the query to create.
     * @return The newly created query object.
     */
    virtual FRHIQuery* CreateQuery(EQueryType InQueryType) = 0;

    /**
     * @brief Creates a GPU fence for GPU->CPU synchronization.
     * @return The newly created fence object.
     */
    virtual FRHIFence* CreateFence() = 0;

    /**
     * @brief Obtains a command context.
     * @return A command context.
     */
    virtual IRHICommandContext* ObtainCommandContext() = 0;

    /**
     * @brief Checks if the current RHI supports unordered access views for the specified format.
     * @param Format Format to check.
     * @return True if unordered access views with the specified format are supported.
     */
    virtual bool QueryUAVFormatSupport(EFormat Format) const = 0;

    /**
     * @brief Retrieves memory statistics from the RHI.
     * @param MemoryType The type of video memory to query.
     * @param OutMemoryInfo Variable to store the memory statistics.
     * @return True if the statistics were retrieved successfully.
     */
    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const = 0;

    /**
     * @brief Gets the result for a query.
     * @param Query Query to get the result from.
     * @param OutResult Variable to store the result.
     * @param Mode Controls synchronization: Available returns current data without blocking, Wait ensures GPU completion first.
     * @return True if the result was retrieved successfully.
     */
    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode = EQueryResultMode::Available) = 0;

    /**
     * @brief Gets the pipeline statistics result for a query.
     * @param Query Pipeline statistics query to get the result from.
     * @param OutResult Variable to store the pipeline statistics.
     * @param Mode Controls synchronization: Available returns current data without blocking, Wait ensures GPU completion first.
     * @return True if the result was retrieved successfully.
     */
    virtual bool GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode = EQueryResultMode::Available) = 0;

    /** @brief Defers destruction of an RHI resource to the deferred deletion code. */
    virtual void EnqueueResourceDeletion(FRHIResource* Resource) = 0;

    /** @return D3D12: IDXGIAdapter*. Vulkan: VkPhysicalDevice. Metal: nullptr (TODO). Null: nullptr. */
    virtual void* GetRHINativeAdapter() = 0;

    /** @return D3D12: ID3D12Device*. Vulkan: VkDevice. Metal: id<MTLDevice>. Null: nullptr. */
    virtual void* GetRHINativeDevice() = 0;

    /** @return D3D12: ID3D12CommandQueue* (direct). Vulkan: VkQueue (graphics). Metal: id<MTLCommandQueue>. Null: nullptr. */
    virtual void* GetRHINativeDirectCommandQueue() = 0;

    /** @return D3D12: ID3D12CommandQueue* (compute). Vulkan: nullptr (TODO). Metal: nullptr (TODO). Null: nullptr. */
    virtual void* GetRHINativeComputeCommandQueue() = 0;

    /** @return D3D12: ID3D12CommandQueue* (copy). Vulkan: nullptr (TODO). Metal: nullptr (TODO). Null: nullptr. */
    virtual void* GetRHINativeCopyCommandQueue() = 0;

    /**
     * @brief Gets the adapter name.
     * @return A string with the adapter name.
     */
    virtual FString GetAdapterName() const = 0;

    /**
     * @brief Gets the current RHI's API type.
     * @return The current RHI's API type.
     */
    virtual ERHIType GetRHIType() const = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
