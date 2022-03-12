#include "RHIInstanceVulkan.h"
#include "VulkanLoader.h"
#include "VulkanTimestampQuery.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanResourceView.h"
#include "VulkanSamplerState.h"
#include "VulkanViewport.h"

#include "Platform/PlatformVulkan.h"

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInstanceVulkan

CRHIInstance* CRHIInstanceVulkan::CreateInstance()
{
    return dbg_new CRHIInstanceVulkan();
}

CRHIInstanceVulkan::CRHIInstanceVulkan()
    : CRHIInstance(ERHIType::Vulkan)
    , Instance(nullptr)
{
}

bool CRHIInstanceVulkan::Initialize(bool bEnableDebug)
{
    SVulkanInstanceDesc InstanceDesc;
    InstanceDesc.RequiredExtensionNames = PlatformVulkan::GetRequiredInstanceExtensions();
    InstanceDesc.RequiredLayerNames     = PlatformVulkan::GetRequiredInstanceLayers();
    InstanceDesc.OptionalExtensionNames = PlatformVulkan::GetOptionalInstanceExtentions();
    InstanceDesc.bEnableValidationLayer = bEnableDebug;
    
    if (bEnableDebug)
    {
        InstanceDesc.RequiredLayerNames.Push("VK_LAYER_KHRONOS_validation");
#if VK_EXT_debug_utils
        InstanceDesc.RequiredExtensionNames.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    }

    Instance = CVulkanInstance::CreateInstance(InstanceDesc);
    if (!Instance)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanDriverInstance");
        return false;
    }
    
    // Load functions that requires an instance here (Order is important)
    if (!LoadInstanceFunctions(Instance.Get()))
    {
        return false;
    }

    SVulkanPhysicalDeviceDesc AdapterDesc;
    AdapterDesc.RequiredExtensionNames = PlatformVulkan::GetRequiredDeviceExtensions();
    AdapterDesc.OptionalExtensionNames = PlatformVulkan::GetOptionalDeviceExtentions();
    AdapterDesc.RequiredFeatures.samplerAnisotropy = VK_TRUE;

    Adapter = CVulkanPhysicalDevice::QueryAdapter(GetInstance(), AdapterDesc);
    if (!Adapter)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    SVulkanDeviceDesc DeviceDesc;
    DeviceDesc.RequiredExtensionNames = AdapterDesc.RequiredExtensionNames;
    DeviceDesc.OptionalExtensionNames = AdapterDesc.OptionalExtensionNames;

    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT BufferDeviceAddressFeatures;

    Device = CVulkanDevice::CreateDevice(GetInstance(), GetAdapter(), DeviceDesc);
    if (!Device)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanPhysicalDevice");
        return false;
    }
    
    // Load functions that requires an device here (Order is important)
    if (!LoadDeviceFunctions(Device.Get()))
    {
        return false;
    }

    DirectCommandQueue = CVulkanQueue::CreateQueue(Device.Get(), EVulkanCommandQueueType::Graphics);
    if (!DirectCommandQueue)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanCommandQueue");
        return false;
    }

    DirectCommandContext = CVulkanCommandContext::CreateCommandContext(Device.Get(), DirectCommandQueue.Get());
    if (!DirectCommandContext)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanCommandContext");
        return false;
    }

    return true;
}

CRHITexture2D* CRHIInstanceVulkan::CreateTexture2D(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture2D(GetDevice(), Format, Width, Height, NumMipLevels, NumSamples, Flags, OptimizedClearValue);
}

CRHITexture2DArray* CRHIInstanceVulkan::CreateTexture2DArray(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture2DArray(GetDevice(), Format, Width, Height, NumMipLevels, NumSamples, NumArraySlices, Flags, OptimizedClearValue);
}

CRHITextureCube* CRHIInstanceVulkan::CreateTextureCube(ERHIFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTextureCube(GetDevice(), Format, Size, NumMipLevels, Flags, OptimizedClearValue);
}

CRHITextureCubeArray* CRHIInstanceVulkan::CreateTextureCubeArray(ERHIFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTextureCubeArray(GetDevice(), Format, Size, NumMipLevels, NumArraySlices, Flags, OptimizedClearValue);
}

CRHITexture3D* CRHIInstanceVulkan::CreateTexture3D(ERHIFormat Format, uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture3D(GetDevice(), Format, Width, Height, Depth, NumMipLevels, Flags, OptimizedClearValue);
}

CRHISamplerStateRef CRHIInstanceVulkan::CreateSamplerState(const class CRHISamplerStateDesc& CreateInfo)
{
    return dbg_new CVulkanSamplerState();
}

CRHIBufferRef CRHIInstanceVulkan::CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    CVulkanBufferRef NewBuffer = dbg_new CVulkanBuffer(GetDevice(), BufferDesc);
    if (NewBuffer && NewBuffer->Initialize())
    {
        return NewBuffer;
    }
    
    return nullptr;
}

CRHIRayTracingScene* CRHIInstanceVulkan::CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return nullptr;
}

CRHIRayTracingGeometry* CRHIInstanceVulkan::CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices)
{
    return nullptr;
}

CRHIShaderResourceView* CRHIInstanceVulkan::CreateShaderResourceView(const SRHIShaderResourceViewDesc& CreateInfo)
{
    return dbg_new CVulkanShaderResourceView();
}

CRHIUnorderedAccessView* CRHIInstanceVulkan::CreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& CreateInfo)
{
    return dbg_new CVulkanUnorderedAccessView();
}

CRHIRenderTargetView* CRHIInstanceVulkan::CreateRenderTargetView(const SRHIRenderTargetViewDesc& CreateInfo)
{
    return dbg_new CVulkanRenderTargetView();
}

CRHIDepthStencilView* CRHIInstanceVulkan::CreateDepthStencilView(const SRHIDepthStencilViewDesc& CreateInfo)
{
    return dbg_new CVulkanDepthStencilView();
}

CRHIComputeShader* CRHIInstanceVulkan::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CVulkanComputeShader>();
}

CRHIVertexShader* CRHIInstanceVulkan::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIVertexShader>();
}

CRHIHullShader* CRHIInstanceVulkan::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIDomainShader* CRHIInstanceVulkan::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIGeometryShader* CRHIInstanceVulkan::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIMeshShader* CRHIInstanceVulkan::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIAmplificationShader* CRHIInstanceVulkan::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIPixelShader* CRHIInstanceVulkan::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIPixelShader>();
}

CRHIRayGenShader* CRHIInstanceVulkan::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayGenShader>();
}

CRHIRayAnyHitShader* CRHIInstanceVulkan::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayAnyHitShader>();
}

CRHIRayClosestHitShader* CRHIInstanceVulkan::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayClosestHitShader>();
}

CRHIRayMissShader* CRHIInstanceVulkan::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayMissShader>();
}

CRHIDepthStencilState* CRHIInstanceVulkan::CreateDepthStencilState(const SRHIDepthStencilStateDesc& CreateInfo)
{
    return dbg_new CVulkanDepthStencilState();
}

CRHIRasterizerState* CRHIInstanceVulkan::CreateRasterizerState(const SRHIRasterizerStateDesc& CreateInfo)
{
    return dbg_new CVulkanRasterizerState();
}

CRHIBlendState* CRHIInstanceVulkan::CreateBlendState(const SRHIBlendStateDesc& CreateInfo)
{
    return dbg_new CVulkanBlendState();
}

CRHIInputLayoutState* CRHIInstanceVulkan::CreateInputLayout(const SRHIInputLayoutStateDesc& CreateInfo)
{
    return dbg_new CVulkanInputLayoutState();
}

CRHIGraphicsPipelineState* CRHIInstanceVulkan::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& CreateInfo)
{
    return dbg_new CVulkanGraphicsPipelineState();
}

CRHIComputePipelineState* CRHIInstanceVulkan::CreateComputePipelineState(const SRHIComputePipelineStateDesc& CreateInfo)
{
    return dbg_new CVulkanComputePipelineState();
}

CRHIRayTracingPipelineState* CRHIInstanceVulkan::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& CreateInfo)
{
    return dbg_new CVulkanRayTracingPipelineState();
}

CRHITimestampQuery* CRHIInstanceVulkan::CreateTimestampQuery()
{
    TSharedRef<CRHITimestampQuery> NewQuery = CVulkanTimestampQuery::CreateQuery(Device.Get());
    return NewQuery.ReleaseOwnership();
}

CRHIViewport* CRHIInstanceVulkan::CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat)
{
    TSharedRef<CVulkanViewport> NewViewport = CVulkanViewport::CreateViewport(Device.Get(), DirectCommandQueue.Get(), WindowHandle, ColorFormat, Width, Height);
    return NewViewport.ReleaseOwnership();
}

String CRHIInstanceVulkan::GetAdapterName() const
{
    VULKAN_ERROR(Adapter != nullptr, "Adapter is not initialized properly");

    VkPhysicalDeviceProperties DeviceProperties = Adapter->GetDeviceProperties();
    return String(DeviceProperties.deviceName);
}

// TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
bool CRHIInstanceVulkan::UAVSupportsFormat(ERHIFormat Format) const
{
    return true;
}

void CRHIInstanceVulkan::CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const
{
    SRHIRayTracingSupport RayTracingSupport;
    RayTracingSupport.MaxRecursionDepth = 0;
    RayTracingSupport.Tier              = ERHIRayTracingTier::NotSupported;

    OutSupport = RayTracingSupport;
}

void CRHIInstanceVulkan::CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const
{
    SRHIShadingRateSupport ShadingRateSupport;
    ShadingRateSupport.ShadingRateImageTileSize = 0;
    ShadingRateSupport.Tier                     = ERHIShadingRateTier::NotSupported;
    
    OutSupport = ShadingRateSupport;
}

