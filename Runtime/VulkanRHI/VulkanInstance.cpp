#include "VulkanInstance.h"
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
// CVulkanInstance

CRHIInstance* CVulkanInstance::CreateInstance()
{
    return dbg_new CVulkanInstance();
}

CVulkanInstance::CVulkanInstance()
    : CRHIInstance(ERHIInstanceApi::Vulkan)
    , Instance(nullptr)
{
}

bool CVulkanInstance::Initialize(bool bEnableDebug)
{
    SVulkanDriverInstanceDesc InstanceDesc;
    InstanceDesc.RequiredExtensionNames = PlatformVulkan::GetRequiredInstanceExtensions();
    InstanceDesc.RequiredLayerNames     = PlatformVulkan::GetRequiredInstanceLayers();
	InstanceDesc.bEnableValidationLayer = bEnableDebug;
	
#if VK_KHR_get_physical_device_properties2
    InstanceDesc.OptionalExtensionNames.Push(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
	
    if (bEnableDebug)
    {
		InstanceDesc.RequiredLayerNames.Push("VK_LAYER_KHRONOS_validation");
		
#if VK_EXT_debug_utils
        InstanceDesc.RequiredExtensionNames.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    }

    Instance = CVulkanDriverInstance::CreateInstance(InstanceDesc);
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
	AdapterDesc.RequiredExtensionNames             = PlatformVulkan::GetRequiredDeviceExtensions();
    AdapterDesc.RequiredFeatures.samplerAnisotropy = VK_TRUE;
    
    // This extension must be enabled on platforms that has it available
    AdapterDesc.OptionalExtensionNames.Push("VK_KHR_portability_subset");

#if VK_KHR_get_memory_requirements2
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance1
    AdapterDesc.OptionalExtensionNames.Push(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance2
    AdapterDesc.OptionalExtensionNames.Push(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance3
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance4
    AdapterDesc.OptionalExtensionNames.Push(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
#endif
#if VK_EXT_descriptor_indexing
	AdapterDesc.OptionalExtensionNames.Push(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
#endif
#if VK_KHR_buffer_device_address
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
#endif
#if VK_KHR_deferred_host_operations
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
#endif
#if VK_KHR_pipeline_library
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
#endif
#if VK_KHR_timeline_semaphore
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
#endif
#if VK_KHR_shader_draw_parameters
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
#endif
#if VK_NV_mesh_shader
	AdapterDesc.OptionalExtensionNames.Push(VK_NV_MESH_SHADER_EXTENSION_NAME);
#endif
#if VK_EXT_memory_budget
	AdapterDesc.OptionalExtensionNames.Push(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
#endif
#if VK_KHR_push_descriptor
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
#endif
#if VK_KHR_ray_query
    AdapterDesc.OptionalExtensionNames.Push(VK_KHR_RAY_QUERY_EXTENSION_NAME);
#endif
#if VK_KHR_ray_tracing_pipeline
    AdapterDesc.OptionalExtensionNames.Push(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
#endif
#if VK_KHR_acceleration_structure
    AdapterDesc.OptionalExtensionNames.Push(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
#endif

	Adapter = CVulkanPhysicalDevice::QueryAdapter(GetInstance(), AdapterDesc);
    if (!Adapter)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    SVulkanDeviceDesc DeviceDesc;
	DeviceDesc.RequiredExtensionNames = AdapterDesc.RequiredExtensionNames;
	DeviceDesc.OptionalExtensionNames = AdapterDesc.OptionalExtensionNames;

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

CRHITexture2D* CVulkanInstance::CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture2D(GetDevice(), Format, Width, Height, NumMipLevels, NumSamples, Flags, OptimizedClearValue);
}

CRHITexture2DArray* CVulkanInstance::CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture2DArray(Format, Width, Height, NumMipLevels, NumSamples, NumArraySlices, Flags, OptimizedClearValue);
}

CRHITextureCube* CVulkanInstance::CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTextureCube(Format, Size, NumMipLevels, Flags, OptimizedClearValue);
}

CRHITextureCubeArray* CVulkanInstance::CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTextureCubeArray(Format, Size, NumMipLevels, NumArraySlices, Flags, OptimizedClearValue);
}

CRHITexture3D* CVulkanInstance::CreateTexture3D(EFormat Format, uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture3D(Format, Width, Height, Depth, NumMipLevels, Flags, OptimizedClearValue);
}

CRHISamplerState* CVulkanInstance::CreateSamplerState(const struct SRHISamplerStateInfo& CreateInfo)
{
    return dbg_new CVulkanSamplerState();
}

CRHIVertexBuffer* CVulkanInstance::CreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return dbg_new TVulkanBuffer<CVulkanVertexBuffer>(NumVertices, Stride, Flags);
}

CRHIIndexBuffer* CVulkanInstance::CreateIndexBuffer(ERHIIndexFormat Format, uint32 NumIndices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return dbg_new TVulkanBuffer<CVulkanIndexBuffer>(Format, NumIndices, Flags);
}

CRHIConstantBuffer* CVulkanInstance::CreateConstantBuffer(uint32 Size, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return dbg_new TVulkanBuffer<CVulkanConstantBuffer>(Size, Flags);
}

CRHIStructuredBuffer* CVulkanInstance::CreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return dbg_new TVulkanBuffer<CVulkanStructuredBuffer>(NumElements, Stride, Flags);
}

CRHIRayTracingScene* CVulkanInstance::CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return nullptr;
}

CRHIRayTracingGeometry* CVulkanInstance::CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer)
{
    return nullptr;
}

CRHIShaderResourceView* CVulkanInstance::CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    return dbg_new CVulkanShaderResourceView();
}

CRHIUnorderedAccessView* CVulkanInstance::CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    return dbg_new CVulkanUnorderedAccessView();
}

CRHIRenderTargetView* CVulkanInstance::CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    return dbg_new CVulkanRenderTargetView();
}

CRHIDepthStencilView* CVulkanInstance::CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
{
    return dbg_new CVulkanDepthStencilView();
}

CRHIComputeShader* CVulkanInstance::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
	return dbg_new TVulkanShader<CVulkanComputeShader>();
}

CRHIVertexShader* CVulkanInstance::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIVertexShader>();
}

CRHIHullShader* CVulkanInstance::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIDomainShader* CVulkanInstance::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIGeometryShader* CVulkanInstance::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIMeshShader* CVulkanInstance::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIAmplificationShader* CVulkanInstance::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIPixelShader* CVulkanInstance::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIPixelShader>();
}

CRHIRayGenShader* CVulkanInstance::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayGenShader>();
}

CRHIRayAnyHitShader* CVulkanInstance::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayAnyHitShader>();
}

CRHIRayClosestHitShader* CVulkanInstance::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayClosestHitShader>();
}

CRHIRayMissShader* CVulkanInstance::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TVulkanShader<CRHIRayMissShader>();
}

CRHIDepthStencilState* CVulkanInstance::CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo)
{
    return dbg_new CVulkanDepthStencilState();
}

CRHIRasterizerState* CVulkanInstance::CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo)
{
    return dbg_new CVulkanRasterizerState();
}

CRHIBlendState* CVulkanInstance::CreateBlendState(const SRHIBlendStateInfo& CreateInfo)
{
    return dbg_new CVulkanBlendState();
}

CRHIInputLayoutState* CVulkanInstance::CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo)
{
    return dbg_new CVulkanInputLayoutState();
}

CRHIGraphicsPipelineState* CVulkanInstance::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo)
{
    return dbg_new CVulkanGraphicsPipelineState();
}

CRHIComputePipelineState* CVulkanInstance::CreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo)
{
    return dbg_new CVulkanComputePipelineState();
}

CRHIRayTracingPipelineState* CVulkanInstance::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo)
{
    return dbg_new CVulkanRayTracingPipelineState();
}

CRHITimestampQuery* CVulkanInstance::CreateTimestampQuery()
{
	TSharedRef<CRHITimestampQuery> NewQuery = CVulkanTimestampQuery::CreateQuery(Device.Get());
	return NewQuery.ReleaseOwnership();
}

CRHIViewport* CVulkanInstance::CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
{
	TSharedRef<CVulkanViewport> NewViewport = CVulkanViewport::CreateViewport(Device.Get(), DirectCommandQueue.Get(), WindowHandle, ColorFormat, Width, Height);
	return NewViewport.ReleaseOwnership();
}

String CVulkanInstance::GetAdapterName() const
{
    VULKAN_ERROR(Adapter != nullptr, "Adapter is not initialized properly");

    VkPhysicalDeviceProperties DeviceProperties = Adapter->GetDeviceProperties();
    return String(DeviceProperties.deviceName);
}

// TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
bool CVulkanInstance::UAVSupportsFormat(EFormat Format) const
{
    return true;
}

void CVulkanInstance::CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const
{
    SRHIRayTracingSupport RayTracingSupport;
    RayTracingSupport.MaxRecursionDepth = 0;
    RayTracingSupport.Tier              = ERHIRayTracingTier::NotSupported;
    OutSupport = RayTracingSupport;
}

void CVulkanInstance::CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const
{
    SRHIShadingRateSupport ShadingRateSupport;
    ShadingRateSupport.ShadingRateImageTileSize = 0;
    ShadingRateSupport.Tier                     = ERHIShadingRateTier::NotSupported;
    OutSupport = ShadingRateSupport;
}

