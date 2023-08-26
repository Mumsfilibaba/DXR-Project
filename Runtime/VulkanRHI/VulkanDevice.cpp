#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"
#include "Core/Misc/ConsoleManager.h"

FVulkanDevice::FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , PhysicalDevice(InAdapter)
    , Device(VK_NULL_HANDLE)
    , UploadHeap(this)
    , bSupportsDepthClip(false)
{
}

FVulkanDevice::~FVulkanDevice()
{
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
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the device extension count");

    TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the device extensions");

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
    IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("Vulkan.VerboseLogging");
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

    // TODO: Check for the availability of these features when the device is created
#if VK_KHR_shader_draw_parameters
    VkPhysicalDeviceShaderDrawParametersFeatures ShaderDrawParametersFeatures;
    FMemory::Memzero(&ShaderDrawParametersFeatures);
    ShaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    
    if (IsExtensionEnabled(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME))
    {
        ShaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;
        DeviceCreateHelper.AddNext(ShaderDrawParametersFeatures);
    }
#endif

#if VK_KHR_buffer_device_address
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR BufferDeviceAddressFeatures;
    FMemory::Memzero(&BufferDeviceAddressFeatures);
    BufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;

    if (IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        BufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        DeviceCreateHelper.AddNext(BufferDeviceAddressFeatures);
    }
#endif

#if VK_KHR_timeline_semaphore
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR TimelineSemaphoreFeatures;
    FMemory::Memzero(&TimelineSemaphoreFeatures);
    TimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;

    if (IsExtensionEnabled(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
    {
        TimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
        DeviceCreateHelper.AddNext(TimelineSemaphoreFeatures);
    }
#endif
    
#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT DepthClipEnableFeatures;
    FMemory::Memzero(&DepthClipEnableFeatures);
    DepthClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;

    if (IsExtensionEnabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME))
    {
        DepthClipEnableFeatures.depthClipEnable = VK_TRUE;
        DeviceCreateHelper.AddNext(DepthClipEnableFeatures);
        bSupportsDepthClip = true;
    }
#endif
    
#if VK_EXT_conservative_rasterization
    if (IsExtensionEnabled(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME))
    {
        bSupportsConservativeRasterization = true;
    }
#endif

    Result = vkCreateDevice(PhysicalDevice->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    VULKAN_CHECK_RESULT(Result, "Failed to create Device");
    return true;
}

bool FVulkanDevice::AllocateMemory(const VkMemoryAllocateInfo& MemoryAllocationInfo, VkDeviceMemory& OutDeviceMemory)
{
    VkResult Result = vkAllocateMemory(Device, &MemoryAllocationInfo, nullptr, &OutDeviceMemory);
    VULKAN_CHECK_RESULT(Result, "vkAllocateMemory failed");
    NumAllocations++;
    
    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetDeviceProperties();
    VULKAN_INFO("[AllocateMemory] Allocated=%d Bytes, NumAllocations = %d/%d", MemoryAllocationInfo.allocationSize, NumAllocations.Load(), DeviceProperties.limits.maxMemoryAllocationCount);
    if (static_cast<uint32>(NumAllocations.Load()) > DeviceProperties.limits.maxMemoryAllocationCount)
    {
        VULKAN_WARNING("Too many allocations");
    }

    return true;
}

void FVulkanDevice::FreeMemory(VkDeviceMemory& OutDeviceMemory)
{
    vkFreeMemory(Device, OutDeviceMemory, nullptr);
    OutDeviceMemory = VK_NULL_HANDLE;
    NumAllocations--;
    
    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetDeviceProperties();
    VULKAN_INFO("[FreeMemory] NumAllocations = %d/%d", NumAllocations.Load(), DeviceProperties.limits.maxMemoryAllocationCount);
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
