#include "VulkanRHI.h"
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
#include "Core/Misc/ConsoleManager.h"

IMPLEMENT_ENGINE_MODULE(FVulkanRHIModule, VulkanRHI);

FRHI* FVulkanRHIModule::CreateRHI()
{
    return new FVulkanRHI();
}


FVulkanRHI* FVulkanRHI::GVulkanRHI = nullptr;

FVulkanRHI::FVulkanRHI()
    : FRHI(ERHIType::Vulkan)
    , Instance(nullptr)
{
    if (!GVulkanRHI)
    {
        GVulkanRHI = this;
    }
}

FVulkanRHI::~FVulkanRHI()
{
    if (GVulkanRHI == this)
    {
        GVulkanRHI = nullptr;
    }
}

bool FVulkanRHI::Initialize()
{
    FVulkanInstanceDesc InstanceDesc;
    InstanceDesc.RequiredExtensionNames = FPlatformVulkan::GetRequiredInstanceExtensions();
    InstanceDesc.RequiredLayerNames     = FPlatformVulkan::GetRequiredInstanceLayers();
    InstanceDesc.OptionalExtensionNames = FPlatformVulkan::GetOptionalInstanceExtentions();
    
    bool bEnableDebugLayer = false;
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }
    
    if (bEnableDebugLayer)
    {
        InstanceDesc.RequiredLayerNames.Add("VK_LAYER_KHRONOS_validation");
#if VK_EXT_debug_utils
        InstanceDesc.RequiredExtensionNames.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    }

    Instance = new FVulkanInstance();
    if (!Instance->Initialize(InstanceDesc))
    {
        VULKAN_ERROR("Failed to initialize VulkanDriverInstance");
        return false;
    }
    
    // Load functions that requires an instance here (Order is important)
    if (!LoadInstanceFunctions(Instance.Get()))
    {
        return false;
    }

    FVulkanPhysicalDeviceDesc AdapterDesc;
    AdapterDesc.RequiredExtensionNames             = FPlatformVulkan::GetRequiredDeviceExtensions();
    AdapterDesc.OptionalExtensionNames             = FPlatformVulkan::GetOptionalDeviceExtentions();
    AdapterDesc.RequiredFeatures.samplerAnisotropy = VK_TRUE;

    PhysicalDevice = new FVulkanPhysicalDevice(GetInstance());
    if (!PhysicalDevice->Initialize(AdapterDesc))
    {
        VULKAN_ERROR("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    FVulkanDeviceDesc DeviceDesc;
    DeviceDesc.RequiredExtensionNames = AdapterDesc.RequiredExtensionNames;
    DeviceDesc.OptionalExtensionNames = AdapterDesc.OptionalExtensionNames;
    DeviceDesc.RequiredFeatures       = AdapterDesc.RequiredFeatures;
    
    Device = new FVulkanDevice(GetInstance(), GetAdapter());
    if (!Device->Initialize(DeviceDesc))
    {
        VULKAN_ERROR("Failed to initialize VulkanPhysicalDevice");
        return false;
    }
    
    // Load functions that requires an device here (Order is important)
    if (!LoadDeviceFunctions(Device.Get()))
    {
        return false;
    }

    GraphicsQueue = new FVulkanQueue(Device.Get(), EVulkanCommandQueueType::Graphics);
    if (!GraphicsQueue->Initialize())
    {
        VULKAN_ERROR("Failed to initialize VulkanCommandQueue");
        return false;
    }

    GraphicsQueue->SetName("Graphics Queue");

    GraphicsCommandContext = new FVulkanCommandContext(Device.Get(), GraphicsQueue.Get());
    if (!GraphicsCommandContext->Initialize())
    {
        VULKAN_ERROR("Failed to initialize VulkanCommandContext");
        return false;
    }

    return true;
}

FRHITexture* FVulkanRHI::RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FVulkanTextureRef NewTexture = new FVulkanTexture(GetDevice(), InDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FVulkanRHI::RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FVulkanBufferRef NewBuffer = new FVulkanBuffer(GetDevice(), InDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FVulkanRHI::RHICreateSamplerState(const FRHISamplerStateDesc& InDesc)
{
    FVulkanSamplerStateRef NewSamplerState = new FVulkanSamplerState(GetDevice(), InDesc);
    if (!NewSamplerState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSamplerState.ReleaseOwnership();
    }
}


FRHIViewport* FVulkanRHI::RHICreateViewport(const FRHIViewportDesc& InDesc)
{
    FVulkanViewportRef NewViewport = new FVulkanViewport(Device.Get(), GraphicsQueue.Get(), InDesc);
    if (!NewViewport->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewViewport.ReleaseOwnership();
    }
}

FRHITimestampQuery* FVulkanRHI::RHICreateTimestampQuery()
{
    FVulkanTimestampQueryRef NewTimestampQuery = new FVulkanTimestampQuery(Device.Get());
    if (!NewTimestampQuery->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewTimestampQuery.ReleaseOwnership();
    }
}

FRHIRayTracingScene* FVulkanRHI::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc)
{
    return nullptr;
}

FRHIRayTracingGeometry* FVulkanRHI::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc)
{
    return nullptr;
}

FRHIShaderResourceView* FVulkanRHI::RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return nullptr;
    }
    
    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), VulkanTexture);
    if (!NewShaderResourceView->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIShaderResourceView* FVulkanRHI::RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return nullptr;
    }
    
    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), VulkanBuffer);
    if (!NewShaderResourceView->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return nullptr;
    }
    
    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), VulkanTexture);
    if (!NewUnorderedAccessView->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return nullptr;
    }
    
    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), VulkanBuffer);
    if (!NewUnorderedAccessView->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIComputeShader* FVulkanRHI::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    FVulkanComputeShaderRef NewShader = new FVulkanComputeShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FVulkanRHI::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    FVulkanVertexShaderRef NewShader = new FVulkanVertexShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FVulkanRHI::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FVulkanRHI::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FVulkanRHI::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FVulkanRHI::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FVulkanRHI::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FVulkanRHI::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    FVulkanPixelShaderRef NewShader = new FVulkanPixelShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FVulkanRHI::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayGenShaderRef NewShader = new FVulkanRayGenShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FVulkanRHI::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayAnyHitShaderRef NewShader = new FVulkanRayAnyHitShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FVulkanRHI::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayClosestHitShaderRef NewShader = new FVulkanRayClosestHitShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FVulkanRHI::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayMissShaderRef NewShader = new FVulkanRayMissShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FVulkanRHI::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
{
    return new FVulkanDepthStencilState(InInitializer);
}

FRHIRasterizerState* FVulkanRHI::RHICreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FVulkanRasterizerState(InDesc);
}

FRHIBlendState* FVulkanRHI::RHICreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FVulkanBlendState(InDesc);
}

FRHIVertexInputLayout* FVulkanRHI::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& InInitializer)
{
    return new FVulkanVertexInputLayout(InInitializer);
}

FRHIGraphicsPipelineState* FVulkanRHI::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    return new FVulkanGraphicsPipelineState();
}

FRHIComputePipelineState* FVulkanRHI::RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    return new FVulkanComputePipelineState();
}

FRHIRayTracingPipelineState* FVulkanRHI::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
{
    return new FVulkanRayTracingPipelineState();
}

void FVulkanRHI::RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const
{
    OutSupport.MaxRecursionDepth = 0;
    OutSupport.Tier = ERHIRayTracingTier::NotSupported;
}

void FVulkanRHI::RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const
{
    OutSupport.ShadingRateImageTileSize = 0;
    OutSupport.Tier = ERHIShadingRateTier::NotSupported;
}

bool FVulkanRHI::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

FString FVulkanRHI::RHIGetAdapterName() const
{
    VULKAN_ERROR_COND(PhysicalDevice != nullptr, "PhysicalDevice is not initialized properly");
    
    VkPhysicalDeviceProperties DeviceProperties = PhysicalDevice->GetDeviceProperties();
    return FString(DeviceProperties.deviceName);
}

