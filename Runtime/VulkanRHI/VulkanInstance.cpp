#include "VulkanInstance.h"
#include "VulkanFunctions.h"

#include "Platform/PlatformVulkanExtensions.h"

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
    InstanceDesc.RequiredExtensionNames = PlatformVulkanExtensions::GetRequiredInstanceExtensions();
    InstanceDesc.RequiredLayerNames     = PlatformVulkanExtensions::GetRequiredInstanceLayers();
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
	AdapterDesc.RequiredExtensionNames = PlatformVulkanExtensions::GetRequiredDeviceExtensions();
    AdapterDesc.RequiredFeatures.samplerAnisotropy = VK_TRUE;
	
#if VK_KHR_get_memory_requirements2
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
#endif

#if VK_KHR_maintenance3
	AdapterDesc.OptionalExtensionNames.Push(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
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
	
	Adapter = CVulkanPhysicalDevice::QueryAdapter(GetInstance(), AdapterDesc);
    if (!Adapter)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    SVulkanDeviceDesc DeviceDesc;
	DeviceDesc.RequiredExtensionNames = AdapterDesc.RequiredExtensionNames;
	DeviceDesc.OptionalExtensionNames = AdapterDesc.OptionalExtensionNames;
    DeviceDesc.bEnableValidationLayer = bEnableDebug;

    Device = CVulkanDevice::CreateDevice(GetInstance(), GetAdapter(), DeviceDesc);
    if (!Device)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    return true;
}

CRHITexture2D* CVulkanInstance::CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return nullptr;
}

CRHITexture2DArray* CVulkanInstance::CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return nullptr;
}

CRHITextureCube* CVulkanInstance::CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return nullptr;
}

CRHITextureCubeArray* CVulkanInstance::CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return nullptr;
}

CRHITexture3D* CVulkanInstance::CreateTexture3D(EFormat Format,uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue)
{
    return nullptr;
}

CRHISamplerState* CVulkanInstance::CreateSamplerState(const struct SRHISamplerStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIVertexBuffer* CVulkanInstance::CreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return nullptr;
}

CRHIIndexBuffer* CVulkanInstance::CreateIndexBuffer(ERHIIndexFormat Format, uint32 NumIndices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return nullptr;
}

CRHIConstantBuffer* CVulkanInstance::CreateConstantBuffer(uint32 Size, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return nullptr;
}

CRHIStructuredBuffer* CVulkanInstance::CreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    return nullptr;
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
    return nullptr;
}

CRHIUnorderedAccessView* CVulkanInstance::CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    return nullptr;
}

CRHIRenderTargetView* CVulkanInstance::CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    return nullptr;
}

CRHIDepthStencilView* CVulkanInstance::CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
{
    return nullptr;
}

CRHIComputeShader* CVulkanInstance::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIVertexShader* CVulkanInstance::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
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
    return nullptr;
}

CRHIRayGenShader* CVulkanInstance::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIRayAnyHitShader* CVulkanInstance::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIRayClosestHitShader* CVulkanInstance::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIRayMissShader* CVulkanInstance::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIDepthStencilState* CVulkanInstance::CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIRasterizerState* CVulkanInstance::CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIBlendState* CVulkanInstance::CreateBlendState(const SRHIBlendStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIInputLayoutState* CVulkanInstance::CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIGraphicsPipelineState* CVulkanInstance::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIComputePipelineState* CVulkanInstance::CreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo)
{
    return nullptr;
}

CRHIRayTracingPipelineState* CVulkanInstance::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo)
{
    return nullptr;
}

CRHITimestampQuery* CVulkanInstance::CreateTimestampQuery()
{
    return nullptr;
}

CRHIViewport* CVulkanInstance::CreateViewport(CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
{
    return nullptr;
}

// TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
bool CVulkanInstance::UAVSupportsFormat(EFormat Format) const
{
    return false;
}

void CVulkanInstance::CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const
{
}

void CVulkanInstance::CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const
{
}

