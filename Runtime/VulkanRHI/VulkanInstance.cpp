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

    if (bEnableDebug)
    {
		InstanceDesc.RequiredLayerNames.Push("VK_LAYER_KHRONOS_validation");
        InstanceDesc.RequiredExtensionNames.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    Instance = CVulkanDriverInstance::CreateInstance(InstanceDesc);
    if (!Instance)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanDriverInstance");
        return false;
    }
	
    // Load functions that requires an instance here (Order is important)
	if (!NVulkan::LoadInstanceFunctions(Instance.Get()))
	{
		return false;
	}

    SVulkanPhysicalDeviceDesc AdapterDesc;
	AdapterDesc.RequiredDeviceLayerNames     = PlatformVulkanExtensions::GetRequiredDeviceLayers();
	AdapterDesc.RequiredDeviceExtensionNames = PlatformVulkanExtensions::GetRequiredDeviceExtensions();

	Adapter = CVulkanPhysicalDevice::QueryAdapter(GetInstance(), AdapterDesc);
    if (!Adapter)
    {
        VULKAN_ERROR_ALWAYS("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    SVulkanDeviceDesc DeviceDesc;
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

