#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"
#include "Core/Misc/ConsoleManager.h"

////////////////////////////////////////////////////
// Global variables that describe different features

VULKANRHI_API bool GVulkanForceBinding              = true;
VULKANRHI_API bool GVulkanForceDedicatedAllocations = true;


FVulkanDevice::FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , PhysicalDevice(InAdapter)
    , Device(VK_NULL_HANDLE)
    , RenderPassCache(this)
    , FramebufferCache(this)
    , UploadHeap(this)
    , MemoryManager(this)
    , bSupportsDepthClip(false)
    , bSupportsConservativeRasterization(false)
{
}

FVulkanDevice::~FVulkanDevice()
{
    // Ensure that all RenderPasses and FrameBuffers are destroyed
    RenderPassCache.ReleaseAll();
    FramebufferCache.ReleaseAll();

    // Ensure that the upload allocator is released before we destroy the device
    UploadHeap.Release();

    // Release all heaps (Which will check for memory leaks)
    MemoryManager.ReleaseMemoryHeaps();
    
    // Destroy the device here
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDestroyDevice(Device, nullptr);
    }
}

bool FVulkanDevice::Initialize(const FVulkanDeviceDesc& DeviceDesc)
{
    VULKAN_ERROR_COND(PhysicalDevice != nullptr, "PhysicalDevice is not initalized correctly");

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

        if (DeviceDesc.RequiredExtensionNames.ContainsWithPredicate(CompareExtension) || DeviceDesc.OptionalExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            EnabledExtensionNames.Add(ExtensionProperty.extensionName);
            ExtensionNames.insert(FString(ExtensionProperty.extensionName));
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

    // Construct the pNext chain 
    FVulkanStructureHelper DeviceCreateHelper(DeviceCreateInfo);
    DeviceCreateHelper.AddNext(DeviceFeatures2);

    VkPhysicalDeviceVulkan11Features DeviceFeaturesVulkan11 = DeviceDesc.RequiredFeatures11;
    DeviceFeaturesVulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    DeviceCreateHelper.AddNext(DeviceFeaturesVulkan11);
    
    VkPhysicalDeviceVulkan12Features DeviceFeaturesVulkan12 = DeviceDesc.RequiredFeatures12;
    DeviceFeaturesVulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    
    // Enable 'bufferDeviceAddress' if available
    const VkPhysicalDeviceVulkan12Features& AvailableDeviceFeaturesVulkan12 = GetPhysicalDevice()->GetDeviceFeaturesVulkan12();
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
    FMemory::Memzero(&Robustness2Features);
    
    Robustness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    
    if (IsExtensionEnabled(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
    {
        const VkPhysicalDeviceRobustness2FeaturesEXT& AvailableRobustness2Features = GetPhysicalDevice()->GetRobustness2Features();
        Robustness2Features.robustImageAccess2  = AvailableRobustness2Features.robustImageAccess2;
        Robustness2Features.robustBufferAccess2 = AvailableRobustness2Features.robustBufferAccess2;
        Robustness2Features.nullDescriptor      = AvailableRobustness2Features.nullDescriptor;

        if (Robustness2Features.robustBufferAccess2)
        {
            DeviceFeatures2.features.robustBufferAccess = VK_TRUE;
        }

        DeviceCreateHelper.AddNext(Robustness2Features);
    }
#endif
    
#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT DepthClipEnableFeatures;
    FMemory::Memzero(&DepthClipEnableFeatures);
    
    DepthClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;

    const VkPhysicalDeviceDepthClipEnableFeaturesEXT& AvailableDepthClipEnableFeatures = GetPhysicalDevice()->GetDepthClipEnableFeatures();
    if (IsExtensionEnabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) && AvailableDepthClipEnableFeatures.depthClipEnable)
    {
        const VkPhysicalDeviceFeatures& AvailableDeviceFeatures = GetPhysicalDevice()->GetDeviceFeatures();
        if (AvailableDeviceFeatures.depthClamp)
        {
            DeviceFeatures2.features.depthClamp     = VK_TRUE;
            DepthClipEnableFeatures.depthClipEnable = VK_TRUE;
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

    Result = vkCreateDevice(PhysicalDevice->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Device");
        return false;
    }

    return true;
}

uint32 FVulkanDevice::GetCommandQueueIndexFromType(EVulkanCommandQueueType Type) const
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
