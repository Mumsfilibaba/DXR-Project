#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"
#include "VulkanCommandContext.h"
#include "Core/Misc/ConsoleManager.h"

////////////////////////////////////////////////////
// Global variables that describe different features

VULKANRHI_API bool GVulkanForceBinding                    = false;
VULKANRHI_API bool GVulkanForceDedicatedAllocations       = false;
VULKANRHI_API bool GVulkanForceDedicatedImageAllocations  = GVulkanForceDedicatedAllocations || true;
VULKANRHI_API bool GVulkanForceDedicatedBufferAllocations = GVulkanForceDedicatedAllocations || false;
VULKANRHI_API bool GVulkanAllowNullDescriptors            = true;

static bool FilterExtensions(const VkExtensionProperties& ExtensionProperty)
{
    if (!GVulkanAllowNullDescriptors && FCString::Strcmp(ExtensionProperty.extensionName, VK_EXT_ROBUSTNESS_2_EXTENSION_NAME) == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

FVulkanDevice::FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , PhysicalDevice(InAdapter)
    , Device(VK_NULL_HANDLE)
    , RenderPassCache(this)
    , FramebufferCache(this)
    , UploadHeap(this)
    , MemoryManager(this)
    , FenceManager(this)
    , PipelineLayoutManager(this)
    , PipelineStateManager(this)
    , DescriptorSetCache(this)
    , DefaultResources()
    , QueryPoolManager(this)
    , bSupportsDepthClip(false)
    , bSupportsConservativeRasterization(false)
    , bSupportsPipelineCacheControl(false)
    , bSupportsAccelerationStructures(false)
{
}

FVulkanDevice::~FVulkanDevice()
{
    // Release default resources
    DefaultResources.Release(*this);

    // Release all samplers
    {
        TScopedLock Lock(SamplerMapCS);

        for (const auto SamplerPair : SamplerMap)
        {
            if (VULKAN_CHECK_HANDLE(SamplerPair.Second))
            {
                vkDestroySampler(GetVkDevice(), SamplerPair.Second, nullptr);
            }
        }

        SamplerMap.Clear();
    }

    // Release the PipelineCache
    PipelineStateManager.SaveCacheData();
    PipelineStateManager.Release();
    
    // Release DescriptorSetCache
    DescriptorSetCache.Release();
    
    // Release all QueryPools
    QueryPoolManager.ReleaseAll();

    // Release all PipelineLayoutManager
    PipelineLayoutManager.Release();

    // Ensure that all RenderPasses and FrameBuffers are destroyed
    RenderPassCache.ReleaseAll();
    FramebufferCache.ReleaseAll();

    // Ensure that the upload allocator is released before we destroy the device
    UploadHeap.Release();

    // Release all Fences
    FenceManager.ReleaseAll();
    
    // Release all heaps (Which will check for memory leaks)
    MemoryManager.ReleaseMemoryHeaps();
    
    // Destroy the device here
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDestroyDevice(Device, nullptr);
    }
}

bool FVulkanDevice::Initialize(const FVulkanDeviceCreateInfo& DeviceDesc)
{
    if (!PhysicalDevice)
    {
        VULKAN_ERROR("PhysicalDevice is not initalized correctly");
        return false;
    }

    VkResult Result = VK_SUCCESS;
    
    // TODO: Device layers
    
    // Verify Extensions
    uint32 DeviceExtensionCount = 0;
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve the device extension count");
        return false;
    }

    TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve the device extensions");
        return false;
    }

    TArray<const CHAR*> EnabledExtensionNames;
    for (const VkExtensionProperties& ExtensionProperty : AvailableDeviceExtensions)
    {
        const auto CompareExtension = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(ExtensionProperty.extensionName, Other) == 0;
        };

        // Filter out some extensions based on CVars etc. (If an extension is available but we want to disable it for debugging or similar)
        if (!FilterExtensions(ExtensionProperty))
        {
            continue;
        }

        if (DeviceDesc.RequiredExtensionNames.ContainsWithPredicate(CompareExtension) || DeviceDesc.OptionalExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            EnabledExtensionNames.Add(ExtensionProperty.extensionName);
            ExtensionNames.Emplace(ExtensionProperty.extensionName);
        }
    }

    for (const CHAR* ExtensionName : DeviceDesc.RequiredExtensionNames)
    {
        const auto CompareExtension = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(ExtensionName, Other) == 0;
        };

        if (!EnabledExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            VULKAN_ERROR("Instance layer '%s' could not be enabled", ExtensionName);
            return false;
        }
    }


    // Log enabled extensions and layers
    IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging");
    if (VerboseVulkan && VerboseVulkan->GetBool())
    {
        if (!EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Device Extensions:");
            for (const CHAR* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO("    %s", ExtensionName);
            }
        }
    }

    QueueIndicies = FVulkanPhysicalDevice::GetQueueFamilyIndices(PhysicalDevice->GetVkPhysicalDevice());
    if (!QueueIndicies)
    {
        VULKAN_ERROR("Failed to query queue indices");
        return false;
    }
    
    VULKAN_INFO("QueueIndicies: Graphics=%d, Compute=%d, Copy=%d", QueueIndicies->GraphicsQueueIndex, QueueIndicies->ComputeQueueIndex, QueueIndicies->CopyQueueIndex);

    TArray<VkDeviceQueueCreateInfo> QueueCreateInfos;

    const TSet<uint32> UniqueQueueFamilies =
    {
        QueueIndicies->GraphicsQueueIndex,
        QueueIndicies->CopyQueueIndex,
        QueueIndicies->ComputeQueueIndex
    };

    const float DefaultQueuePriority = 0.0f;
    for (uint32 QueueFamiliy : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo;
        FMemory::Memzero(&QueueCreateInfo);
        
        QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.pQueuePriorities = &DefaultQueuePriority;
        QueueCreateInfo.queueFamilyIndex = QueueFamiliy;
        QueueCreateInfo.queueCount       = 1;

        QueueCreateInfos.Add(QueueCreateInfo);
    }

    VkDeviceCreateInfo DeviceCreateInfo;
    FMemory::Memzero(&DeviceCreateInfo);

    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.enabledLayerCount       = 0;
    DeviceCreateInfo.ppEnabledLayerNames     = nullptr;
    DeviceCreateInfo.enabledExtensionCount   = EnabledExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = EnabledExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = QueueCreateInfos.Size();
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.Data();
    
    VkPhysicalDeviceFeatures2 DeviceFeatures2;
    FMemory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    DeviceFeatures2.features = DeviceDesc.RequiredFeatures;

    const VkPhysicalDeviceFeatures& AvailableDeviceFeatures = GetPhysicalDevice()->GetFeatures();
    if (AvailableDeviceFeatures.robustBufferAccess)
    {
        DeviceFeatures2.features.robustBufferAccess = VK_TRUE;
    }
    
    // Construct the pNext chain
    FVulkanStructureHelper DeviceCreateHelper(DeviceCreateInfo);
    DeviceCreateHelper.AddNext(DeviceFeatures2);

    VkPhysicalDeviceVulkan11Features DeviceFeaturesVulkan11 = DeviceDesc.RequiredFeatures11;
    DeviceFeaturesVulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    DeviceCreateHelper.AddNext(DeviceFeaturesVulkan11);
    
    VkPhysicalDeviceVulkan12Features DeviceFeaturesVulkan12 = DeviceDesc.RequiredFeatures12;
    DeviceFeaturesVulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    
    // Enable 'bufferDeviceAddress' if available
    const VkPhysicalDeviceVulkan12Features& AvailableDeviceFeaturesVulkan12 = GetPhysicalDevice()->GetFeaturesVulkan12();
    if (AvailableDeviceFeaturesVulkan12.bufferDeviceAddress)
    {
        DeviceFeaturesVulkan12.bufferDeviceAddress = VK_TRUE;
    }
    // Enable 'timelineSemaphore' if available
    if (AvailableDeviceFeaturesVulkan12.timelineSemaphore)
    {
        DeviceFeaturesVulkan12.timelineSemaphore = VK_TRUE;
    }
    // Enable 'descriptorIndexing' if available
    if (AvailableDeviceFeaturesVulkan12.descriptorIndexing)
    {
        DeviceFeaturesVulkan12.descriptorIndexing = VK_TRUE;
    }
    
    DeviceCreateHelper.AddNext(DeviceFeaturesVulkan12);

    // Check and enable extension features
#if VK_EXT_robustness2
    VkPhysicalDeviceRobustness2FeaturesEXT Robustness2Features;
    if (IsExtensionEnabled(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
    {
        const VkPhysicalDeviceRobustness2FeaturesEXT& AvailableRobustness2Features = GetPhysicalDevice()->GetRobustness2Features();

        FMemory::Memzero(&Robustness2Features);
        Robustness2Features.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
        Robustness2Features.robustImageAccess2  = AvailableRobustness2Features.robustImageAccess2;
        Robustness2Features.robustBufferAccess2 = AvailableRobustness2Features.robustBufferAccess2;
        Robustness2Features.nullDescriptor      = AvailableRobustness2Features.nullDescriptor;
        DeviceCreateHelper.AddNext(Robustness2Features);
    }
#endif
    
#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT DepthClipEnableFeatures;
    if (IsExtensionEnabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME))
    {
        const VkPhysicalDeviceDepthClipEnableFeaturesEXT& AvailableDepthClipEnableFeatures = GetPhysicalDevice()->GetDepthClipEnableFeatures();
        if (AvailableDepthClipEnableFeatures.depthClipEnable && AvailableDeviceFeatures.depthClamp)
        {
            FMemory::Memzero(&DepthClipEnableFeatures);
            DepthClipEnableFeatures.sType           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
            DepthClipEnableFeatures.depthClipEnable = VK_TRUE;
            DeviceFeatures2.features.depthClamp     = VK_TRUE;
            DeviceCreateHelper.AddNext(DepthClipEnableFeatures);
            bSupportsDepthClip = true;
        }
    }
#endif

#if VK_EXT_conservative_rasterization
    if (IsExtensionEnabled(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME))
    {
        bSupportsConservativeRasterization = true;
    }
#endif

#if VK_EXT_pipeline_creation_cache_control
    VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT PipelineCreationCacheControlFeatures;
    if (IsExtensionEnabled(VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME))
    {
        const VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT& AvailablePipelineCreationCacheControlFeatures = GetPhysicalDevice()->GetPipelineCreationCacheControlFeatures();
        if (AvailablePipelineCreationCacheControlFeatures.pipelineCreationCacheControl)
        {
            FMemory::Memzero(&PipelineCreationCacheControlFeatures);
            PipelineCreationCacheControlFeatures.sType                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT;
            PipelineCreationCacheControlFeatures.pipelineCreationCacheControl = VK_TRUE;
            DeviceCreateHelper.AddNext(PipelineCreationCacheControlFeatures);
            bSupportsPipelineCacheControl = true;
        }
    }
#endif

#if VK_KHR_acceleration_structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures;
    if (IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
    {
        const VkPhysicalDeviceAccelerationStructureFeaturesKHR& AvailableAccelerationStructureFeatures = GetPhysicalDevice()->GetAccelerationStructureFeatures();
        if (AvailableAccelerationStructureFeatures.accelerationStructure)
        {
            FMemory::Memzero(&AccelerationStructureFeatures);
            AccelerationStructureFeatures.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            AccelerationStructureFeatures.accelerationStructure = VK_TRUE;
            DeviceCreateHelper.AddNext(AccelerationStructureFeatures);
            bSupportsAccelerationStructures = true;
        }
    }
#endif

#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures;
    if (IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
    {
        const VkPhysicalDeviceRayTracingPipelineFeaturesKHR& AvailableRayTracingPipelineFeatures = GetPhysicalDevice()->GetRayTracingPipelineFeatures();
        if (AvailableRayTracingPipelineFeatures.rayTracingPipeline)
        {
            FMemory::Memzero(&RayTracingPipelineFeatures);
            RayTracingPipelineFeatures.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            RayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
            DeviceCreateHelper.AddNext(RayTracingPipelineFeatures);
        }
    }
#endif

    Result = vkCreateDevice(PhysicalDevice->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Device");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanDevice::PostLoaderInitalize()
{
    // Initialize PipelineCache
    if (!PipelineStateManager.Initialize())
    {
        return false;
    }

    // Initialize hardware support globals
    VkPhysicalDevice PhysicalDeviceHandle = GetPhysicalDevice()->GetVkPhysicalDevice();
    if (IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
    {
        VkPhysicalDeviceProperties2 DeviceProperties2;
        FMemory::Memzero(&DeviceProperties2);

        FVulkanStructureHelper DevicePropertiesHelper(DeviceProperties2);
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties;
        FMemory::Memzero(&RayTracingPipelineProperties);

        DevicePropertiesHelper.AddNext(RayTracingPipelineProperties);
        RayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        // Check if RayQueries are supported, then the Tier is kind of like Tier 1.1 (Inline RayTracing in DXR)
        if (IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
            GRHIRayTracingTier = ERayTracingTier::Tier1_1;
        }
        else
        {
            GRHIRayTracingTier = ERayTracingTier::Tier1;
        }

        GRHIRayTracingMaxRecursionDepth = RayTracingPipelineProperties.maxRayRecursionDepth;
    }
    else
    {
        GRHIRayTracingTier = ERayTracingTier::NotSupported;
    }

    GRHISupportsRayTracing = GRHIRayTracingTier != ERayTracingTier::NotSupported;

    if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        VkPhysicalDeviceProperties2 DeviceProperties2;
        FMemory::Memzero(&DeviceProperties2);

        FVulkanStructureHelper DevicePropertiesHelper(DeviceProperties2);
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceFragmentShadingRatePropertiesKHR FragmentShadingRateProperties;
        FMemory::Memzero(&FragmentShadingRateProperties);

        DevicePropertiesHelper.AddNext(FragmentShadingRateProperties);
        FragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        // TODO: Finish this part
        GRHIShadingRateImageTileSize = 0;
        GRHIShadingRateTier = EShadingRateTier::NotSupported;
    }
    else
    {
        GRHIShadingRateTier = EShadingRateTier::NotSupported;
    }

    GRHISupportsVRS = GRHIShadingRateTier != EShadingRateTier::NotSupported;
    return true;
}

// Initialize default resources that are just for null bindings
bool FVulkanDevice::InitializeDefaultResources(FVulkanCommandContext& CommandContext)
{
    // Create the resources
    if (!DefaultResources.Initialize(*this))
    {
        VULKAN_ERROR("Failed to create DefaultResources");
        return false;
    }

    // If null-descriptors are supported then we can return here, since we have no DefaultResources to upload
    if (FVulkanRobustness2EXT::SupportsNullDescriptors())
    {
        return true;
    }

    CommandContext.ObtainCommandBuffer();

    VkBuffer DefaultBuffer = DefaultResources.NullBuffer;
    CommandContext.GetCommandBuffer()->FillBuffer(DefaultBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);

    VkImage DefaultImage = DefaultResources.NullImage;

    FVulkanImageTransitionBarrier TransitionBarrier;
    TransitionBarrier.Image                           = DefaultImage;
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_UNDEFINED;
    TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_NONE;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VK_FORMAT_R8G8B8A8_UNORM);
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
    TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    CommandContext.GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);

    VkBufferImageCopy BufferImageCopy;
    BufferImageCopy.bufferOffset                    = 0;
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = TransitionBarrier.SubresourceRange.aspectMask;
    BufferImageCopy.imageSubresource.mipLevel       = 0;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { 0, 0, 0 };
    BufferImageCopy.imageExtent                     = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    CommandContext.GetCommandBuffer()->CopyBufferToImage(DefaultBuffer, DefaultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

    TransitionBarrier.PreviousLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionBarrier.NewLayout      = VK_IMAGE_LAYOUT_GENERAL;
    TransitionBarrier.SrcAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.DstAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.SrcStageMask   = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.DstStageMask   = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    CommandContext.GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
    CommandContext.FinishCommandBuffer(true);
    return true;
}

bool FVulkanDevice::FindOrCreateSampler(const VkSamplerCreateInfo& SamplerCreateInfo, VkSampler& OutSampler)
{
    CHECK(SamplerCreateInfo.pNext == nullptr);

    TScopedLock Lock(SamplerMapCS);

    FVulkanHashableSamplerCreateInfo HashableCreateInfo;
    HashableCreateInfo.Flags                   = SamplerCreateInfo.flags;
    HashableCreateInfo.MagFilter               = SamplerCreateInfo.magFilter;
    HashableCreateInfo.MinFilter               = SamplerCreateInfo.minFilter;
    HashableCreateInfo.MipmapMode              = SamplerCreateInfo.mipmapMode;
    HashableCreateInfo.AddressModeU            = SamplerCreateInfo.addressModeU;
    HashableCreateInfo.AddressModeV            = SamplerCreateInfo.addressModeV;
    HashableCreateInfo.AddressModeW            = SamplerCreateInfo.addressModeW;
    HashableCreateInfo.MipLodBias              = SamplerCreateInfo.mipLodBias;
    HashableCreateInfo.AnisotropyEnable        = SamplerCreateInfo.anisotropyEnable;
    HashableCreateInfo.MaxAnisotropy           = SamplerCreateInfo.maxAnisotropy;
    HashableCreateInfo.CompareEnable           = SamplerCreateInfo.compareEnable;
    HashableCreateInfo.CompareOp               = SamplerCreateInfo.compareOp;
    HashableCreateInfo.MinLod                  = SamplerCreateInfo.minLod;
    HashableCreateInfo.MaxLod                  = SamplerCreateInfo.maxLod;
    HashableCreateInfo.BorderColor             = SamplerCreateInfo.borderColor;
    HashableCreateInfo.UnnormalizedCoordinates = SamplerCreateInfo.unnormalizedCoordinates;

    if (VkSampler* Sampler = SamplerMap.Find(HashableCreateInfo))
    {
        OutSampler = *Sampler;
        return true;
    }

    VkResult Result = vkCreateSampler(GetVkDevice(), &SamplerCreateInfo, nullptr, &OutSampler);
    if (VULKAN_FAILED(Result))
    {
        OutSampler = VK_NULL_HANDLE;
        VULKAN_ERROR("Failed to create sampler");
        return false;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("Sampler %d", SamplerMap.Size());
        FVulkanDebugUtilsEXT::SetObjectName(GetVkDevice(), DebugName.Data(), OutSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    SamplerMap.Add(HashableCreateInfo, OutSampler);
    return true;
}

uint32 FVulkanDevice::GetQueueIndexFromType(EVulkanCommandQueueType Type) const
{
    if (Type == EVulkanCommandQueueType::Graphics)
    {
        return QueueIndicies->GraphicsQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Compute)
    {
        return QueueIndicies->ComputeQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Copy)
    {
        return QueueIndicies->CopyQueueIndex;
    }
    else
    {
        VULKAN_ERROR("Invalid CommandQueueType");
        return (~0U);
    }
}

bool FVulkanDefaultResources::Initialize(FVulkanDevice& Device)
{
    // We only need to actually create these resources if we don't support null-descriptors
    if (!FVulkanRobustness2EXT::SupportsNullDescriptors())
    {
        if (!InitializeBuffersAndImages(Device))
        {
            return false;
        }
    }

    // Create a NullSampler
    VkSamplerCreateInfo SamplerCreateInfo;
    FMemory::Memzero(&SamplerCreateInfo);

    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.mipLodBias              = 0.0f;
    SamplerCreateInfo.anisotropyEnable        = VK_FALSE;
    SamplerCreateInfo.maxAnisotropy           = 1.0f;
    SamplerCreateInfo.compareEnable           = VK_FALSE;
    SamplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
    SamplerCreateInfo.minLod                  = 0.0f;
    SamplerCreateInfo.maxLod                  = VK_LOD_CLAMP_NONE;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    if (!Device.FindOrCreateSampler(SamplerCreateInfo, NullSampler))
    {
        VULKAN_ERROR("vkCreateSampler failed");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullSampler", NullSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    return true;
}

bool FVulkanDefaultResources::InitializeBuffersAndImages(FVulkanDevice& Device)
{
    // Create NullBuffer
    VkBufferCreateInfo BufferCreateInfo;
    FMemory::Memzero(&BufferCreateInfo);

    BufferCreateInfo.usage = 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = VULKAN_DEFAULT_BUFFER_NUM_BYTES;

    VkResult Result = vkCreateBuffer(Device.GetVkDevice(), &BufferCreateInfo, nullptr, &NullBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Buffer");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullBuffer", NullBuffer, VK_OBJECT_TYPE_BUFFER);
    }

    // Allocate memory based on the buffer
    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(NullBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, false, NullBufferMemory))
    {
        VULKAN_ERROR("Failed to allocate buffer memory");
        return false;
    }

    // Create a NullImage
    constexpr VkExtent3D NullExtent = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    VkImageCreateInfo ImageCreateInfo;
    FMemory::Memzero(&ImageCreateInfo);

    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType             = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ImageCreateInfo.format                = VK_FORMAT_R8G8B8A8_UNORM;
    ImageCreateInfo.extent                = NullExtent;
    ImageCreateInfo.mipLevels             = 1;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.arrayLayers           = 1;

    Result = vkCreateImage(Device.GetVkDevice(), &ImageCreateInfo, nullptr, &NullImage);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create image");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImage", NullImage, VK_OBJECT_TYPE_IMAGE);
    }

    if (!MemoryManager.AllocateImageMemory(NullImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, false, NullImageMemory))
    {
        VULKAN_ERROR("Failed to allocate ImageMemory");
        return false;
    }

    // Create NullImageView
    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                           = 0;
    ImageViewCreateInfo.format                          = ImageCreateInfo.format;
    ImageViewCreateInfo.image                           = NullImage;
    ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    Result = vkCreateImageView(Device.GetVkDevice(), &ImageViewCreateInfo, nullptr, &NullImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateImageView failed");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImageView", NullImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    return true;
}

void FVulkanDefaultResources::Release(FVulkanDevice& Device)
{
    VkDevice VulkanDevice = Device.GetVkDevice();
    if (VULKAN_CHECK_HANDLE(NullBuffer))
    {
        vkDestroyBuffer(VulkanDevice, NullBuffer, nullptr);
        NullBuffer = VK_NULL_HANDLE;

        FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
        MemoryManager.Free(NullBufferMemory);
    }

    if (VULKAN_CHECK_HANDLE(NullImageView))
    {
        vkDestroyImageView(VulkanDevice, NullImageView, nullptr);
        NullImageView = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(NullImage))
    {
        vkDestroyImage(VulkanDevice, NullImage, nullptr);
        NullImage = VK_NULL_HANDLE;

        FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
        MemoryManager.Free(NullImageMemory);
    }

    // All samplers are cached and deleted by the device
    NullSampler = VK_NULL_HANDLE;
}
