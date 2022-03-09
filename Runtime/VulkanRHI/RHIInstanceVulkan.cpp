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
#if VK_KHR_dedicated_allocation
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
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

CRHITexture2D* CRHIInstanceVulkan::CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture2D(GetDevice(), Format, Width, Height, NumMipLevels, NumSamples, Flags, OptimizedClearValue);
}

CRHITexture2DArray* CRHIInstanceVulkan::CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture2DArray(GetDevice(), Format, Width, Height, NumMipLevels, NumSamples, NumArraySlices, Flags, OptimizedClearValue);
}

CRHITextureCube* CRHIInstanceVulkan::CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTextureCube(GetDevice(), Format, Size, NumMipLevels, Flags, OptimizedClearValue);
}

CRHITextureCubeArray* CRHIInstanceVulkan::CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTextureCubeArray(GetDevice(), Format, Size, NumMipLevels, NumArraySlices, Flags, OptimizedClearValue);
}

CRHITexture3D* CRHIInstanceVulkan::CreateTexture3D(EFormat Format, uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return dbg_new CVulkanTexture3D(GetDevice(), Format, Width, Height, Depth, NumMipLevels, Flags, OptimizedClearValue);
}

CRHISamplerStateRef CRHIInstanceVulkan::CreateSamplerState(const struct CRHISamplerStateDesc& CreateInfo)
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

CRHIRayTracingScene* CRHIInstanceVulkan::CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return nullptr;
}

CRHIRayTracingGeometry* CRHIInstanceVulkan::CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer)
{
    return nullptr;
}

CRHIShaderResourceView* CRHIInstanceVulkan::CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    return dbg_new CVulkanShaderResourceView();
}

CRHIUnorderedAccessView* CRHIInstanceVulkan::CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    return dbg_new CVulkanUnorderedAccessView();
}

CRHIRenderTargetView* CRHIInstanceVulkan::CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    return dbg_new CVulkanRenderTargetView();
}

CRHIDepthStencilView* CRHIInstanceVulkan::CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
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

CRHIDepthStencilState* CRHIInstanceVulkan::CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo)
{
    return dbg_new CVulkanDepthStencilState();
}

CRHIRasterizerState* CRHIInstanceVulkan::CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo)
{
    return dbg_new CVulkanRasterizerState();
}

CRHIBlendState* CRHIInstanceVulkan::CreateBlendState(const SRHIBlendStateInfo& CreateInfo)
{
    return dbg_new CVulkanBlendState();
}

CRHIInputLayoutState* CRHIInstanceVulkan::CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo)
{
    return dbg_new CVulkanInputLayoutState();
}

CRHIGraphicsPipelineState* CRHIInstanceVulkan::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo)
{
    return dbg_new CVulkanGraphicsPipelineState();
}

CRHIComputePipelineState* CRHIInstanceVulkan::CreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo)
{
    return dbg_new CVulkanComputePipelineState();
}

CRHIRayTracingPipelineState* CRHIInstanceVulkan::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo)
{
    return dbg_new CVulkanRayTracingPipelineState();
}

CRHITimestampQuery* CRHIInstanceVulkan::CreateTimestampQuery()
{
	TSharedRef<CRHITimestampQuery> NewQuery = CVulkanTimestampQuery::CreateQuery(Device.Get());
	return NewQuery.ReleaseOwnership();
}

CRHIViewport* CRHIInstanceVulkan::CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
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
bool CRHIInstanceVulkan::UAVSupportsFormat(EFormat Format) const
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

