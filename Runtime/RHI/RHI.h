#pragma once
#include "Core/Modules/ModuleManager.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct RHI_API FRHIModule : public IModule
{
    virtual ~FRHIModule() = default;

    /**
     * @brief Creates the RHI device instance
     * @return Returns the newly created RHI device instance
     */
    virtual FRHIDevice* CreateDevice() { return nullptr; }
};

struct RHI
{
    // -------------------------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------------------------

    /** @brief Initializes the RHI Interface */
    static RHI_API bool Initialize();

    /** @brief Releases the RHI Interface */
    static RHI_API void Release();

    /** @return Returns true if the RHI is initialized */
    static FORCEINLINE bool IsInitialized()
    {
        return Device != nullptr;
    }

    // -------------------------------------------------------------------------------------------
    // Create functions (Forward to RHI::Device)
    // -------------------------------------------------------------------------------------------

    static FORCEINLINE FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState = EResourceAccess::Common, const IRHITextureData* InInitialData = nullptr)
    {
        return Device->CreateTexture(InTextureDesc, InInitialState, InInitialData);
    }

    static FORCEINLINE FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState = EResourceAccess::Common, const void* InInitialData = nullptr)
    {
        return Device->CreateBuffer(InBufferDesc, InInitialState, InInitialData);
    }

    static FORCEINLINE FRHISamplerState* CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
    {
        return Device->CreateSamplerState(InSamplerDesc);
    }

    static FORCEINLINE FRHISwapChain* CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
    {
        return Device->CreateSwapChain(InSwapChainDesc);
    }

    static FORCEINLINE FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
    {
        return Device->CreateSceneAccelerationStructure(InSceneDesc);
    }

    static FORCEINLINE FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    {
        return Device->CreateGeometryAccelerationStructure(InGeometryDesc);
    }

    static FORCEINLINE FRHIShaderResourceView* CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
    {
        return Device->CreateShaderResourceView(InResource, InDesc);
    }

    static FORCEINLINE FRHIUnorderedAccessView* CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
    {
        return Device->CreateUnorderedAccessView(InResource, InDesc);
    }

    static FORCEINLINE FRHIRenderTargetView* CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
    {
        return Device->CreateRenderTargetView(InResource, InDesc);
    }

    static FORCEINLINE FRHIDepthStencilView* CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
    {
        return Device->CreateDepthStencilView(InResource, InDesc);
    }

    static FORCEINLINE FRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateComputeShader(ShaderCode);
    }

    static FORCEINLINE FRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateVertexShader(ShaderCode);
    }

    static FORCEINLINE FRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateHullShader(ShaderCode);
    }

    static FORCEINLINE FRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateDomainShader(ShaderCode);
    }

    static FORCEINLINE FRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateGeometryShader(ShaderCode);
    }

    static FORCEINLINE FRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateMeshShader(ShaderCode);
    }

    static FORCEINLINE FRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateAmplificationShader(ShaderCode);
    }

    static FORCEINLINE FRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreatePixelShader(ShaderCode);
    }

    static FORCEINLINE FRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateRayGenShader(ShaderCode);
    }

    static FORCEINLINE FRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateRayAnyHitShader(ShaderCode);
    }

    static FORCEINLINE FRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateRayClosestHitShader(ShaderCode);
    }

    static FORCEINLINE FRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode)
    {
        return Device->CreateRayMissShader(ShaderCode);
    }

    static FORCEINLINE FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
    {
        return Device->CreateDepthStencilState(InDesc);
    }

    static FORCEINLINE FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
    {
        return Device->CreateRasterizerState(InDesc);
    }

    static FORCEINLINE FRHIBlendState* CreateBlendState(const FRHIBlendStateDesc& InDesc)
    {
        return Device->CreateBlendState(InDesc);
    }

    static FORCEINLINE FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
    {
        return Device->CreateInputLayout(InInputElements);
    }

    static FORCEINLINE FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
    {
        return Device->CreateGraphicsPipelineState(InDesc);
    }

    static FORCEINLINE FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
    {
        return Device->CreateComputePipelineState(InDesc);
    }

    static FORCEINLINE FRHIMeshletPipelineState* CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
    {
        return Device->CreateMeshletPipelineState(InDesc);
    }

    static FORCEINLINE FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
    {
        return Device->CreateRayTracingPipelineState(InDesc);
    }

    static FORCEINLINE FRHIQuery* CreateQuery(EQueryType InQueryType)
    {
        return Device->CreateQuery(InQueryType);
    }

    static FORCEINLINE FRHIFence* CreateFence()
    {
        return Device->CreateFence();
    }

    // -------------------------------------------------------------------------------------------
    // Active Device
    // -------------------------------------------------------------------------------------------

    /** @brief The active RHI device. Set during RHI::Initialize() and cleared by RHI::Release(). */
    static RHI_API FRHIDevice* Device;

    // -------------------------------------------------------------------------------------------
    // Shader / Pipeline Features
    // -------------------------------------------------------------------------------------------

    /** Whether the device supports geometry shaders */
    static RHI_API bool bSupportsGeometryShaders;

    /** Whether SV_RenderTargetArrayIndex is supported from the vertex shader stage */
    static RHI_API bool bSupportRenderTargetArrayIndexFromVertexShader;

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------

    /** Whether view instancing is supported */
    static RHI_API bool bSupportsViewInstancing;

    /** Maximum number of view instances supported */
    static RHI_API uint32 MaxViewInstanceCount;

    // -------------------------------------------------------------------------------------------
    // Hardware Ray Tracing
    // -------------------------------------------------------------------------------------------

    /** Whether hardware-accelerated ray tracing is supported */
    static RHI_API bool bSupportsRayTracing;

    /** Ray tracing tier support (e.g., Tier 1.0, 1.1, etc.) */
    static RHI_API ERayTracingTier RayTracingTier;

    /** Maximum recursion depth supported for ray tracing pipelines */
    static RHI_API uint32 RayTracingMaxRecursionDepth;

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------

    /** Whether hardware Variable Rate Shading is supported */
    static RHI_API bool bSupportsVRS;

    /** Shading rate tier (Tier1, Tier2, etc.) */
    static RHI_API EShadingRateTier ShadingRateTier;

    /** Shading rate image tile size (e.g., 16x16) */
    static RHI_API uint32 ShadingRateImageTileSize;

    // -------------------------------------------------------------------------------------------
    // Draw Indirect
    // -------------------------------------------------------------------------------------------

    /** Whether indirect draw calls are supported */
    static RHI_API bool bSupportDrawIndirect;

    /** Whether multi-draw indirect is supported */
    static RHI_API bool bSupportMultiDrawIndirect;

    /** Maximum number of draws per indirect call */
    static RHI_API uint32 MaxDrawIndirectCount;

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits
    // -------------------------------------------------------------------------------------------

    // --- 1D Textures ---

    /** Maximum width of a 1D texture */
    static RHI_API uint32 MaxTexture1DSize;

    /** Maximum number of array layers for 1D textures */
    static RHI_API uint32 MaxTexture1DArrayLayers;


    // --- 2D Textures ---

    /** Maximum width or height of a 2D texture */
    static RHI_API uint32 MaxTexture2DSize;

    /** Maximum number of array layers for 2D textures */
    static RHI_API uint32 MaxTexture2DArrayLayers;


    // --- 3D Textures ---

    /** Maximum 3D texture width */
    static RHI_API uint32 MaxTexture3DWidth;

    /** Maximum 3D texture height */
    static RHI_API uint32 MaxTexture3DHeight;

    /** Maximum 3D texture depth */
    static RHI_API uint32 MaxTexture3DDepth;


    // --- Cube Textures ---

    /** Maximum cube map face resolution */
    static RHI_API uint32 MaxCubeTextureSize;

    /** Maximum number of cube maps in a cube texture array (arrayLayers / 6) */
    static RHI_API uint32 MaxCubeArrayCount;

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------

    /** Maximum buffer size in bytes */
    static RHI_API uint64 MaxBufferSize;

    // --- Constant / Uniform Buffers ---

    /** Maximum size of a constant/uniform buffer binding */
    static RHI_API uint32 MaxConstantBufferSize;

    // --- Storage / Structured Buffers ---

    /** Maximum size of a storage buffer binding */
    static RHI_API uint64 MaxStorageBufferSize;

    /** Minimum stride for structured buffers (bytes) */
    static RHI_API uint32 StructuredBufferMinStride;

    /** Maximum stride for structured buffers (bytes) */
    static RHI_API uint32 StructuredBufferMaxStride;

    /** Required alignment for raw/byte-address buffers (SRV/UAV) */
    static RHI_API uint32 RawBufferRequiredAlignment;

    // -------------------------------------------------------------------------------------------
    // Dynamic State
    // -------------------------------------------------------------------------------------------

    /** Whether dynamic depth bias (RSSetDepthBias) is supported */
    static RHI_API bool bSupportsDynamicDepthBias;

    /** Whether stream output / transform feedback is supported */
    static RHI_API bool bSupportsStreamOutput;

    // -------------------------------------------------------------------------------------------
    // Query Support
    // -------------------------------------------------------------------------------------------

    /** Whether GPU timestamp queries are supported */
    static RHI_API bool bSupportsTimestampQueries;

    /** Whether pipeline statistics queries are supported */
    static RHI_API bool bSupportsPipelineStatisticsQueries;

    /** Whether the backend can filter out GPU idle bubbles from timestamp results */
    static RHI_API bool bSupportsGPUTimestampBubblesRemoval;

    // -------------------------------------------------------------------------------------------
    // Swap-Chain Defaults
    // -------------------------------------------------------------------------------------------

    /**
     * The format the active RHI backend picks when an FRHISwapChainDesc is created with
     * EFormat::Unknown. Populated during backend initialisation from the per-backend CVar
     * (D3D12RHI.DefaultBackBufferFormat / VulkanRHI.DefaultBackBufferFormat). Read-only
     * outside of init - use it when a caller needs to know up front what a swap chain
     * created with Unknown will end up with.
     */
    static RHI_API EFormat DefaultSwapChainFormat;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
