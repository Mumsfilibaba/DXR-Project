#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"

#include "Core/Debug/Console/ConsoleManager.h"

static const auto RawStringComparator = [](const char* Lhs, const char* Rhs) -> bool
{
    return StringMisc::Compare(Lhs, Rhs) == 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

CVulkanDeviceRef CVulkanDevice::CreateDevice(CVulkanInstance* InInstance, CVulkanPhysicalDevice* InAdapter, const SVulkanDeviceDesc& DeviceDesc)
{
    VULKAN_ERROR(InInstance != nullptr, "Instance cannot be nullptr");
    VULKAN_ERROR(InAdapter  != nullptr, "Adapter cannot be nullptr");

    CVulkanDeviceRef NewDevice = dbg_new CVulkanDevice(InInstance, InAdapter);
    if (NewDevice && NewDevice->Initialize(DeviceDesc))
    {
        return NewDevice;
    }
    else
    {
        return nullptr;
    }
}

CVulkanDevice::CVulkanDevice(CVulkanInstance* InInstance, CVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , Adapter(InAdapter)
    , Device(VK_NULL_HANDLE)
{
}

CVulkanDevice::~CVulkanDevice()
{
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDestroyDevice(Device, nullptr);
    }
}

bool CVulkanDevice::Initialize(const SVulkanDeviceDesc& DeviceDesc)
{
    VULKAN_ERROR(Adapter != nullptr, "Adapter is not initalized correctly");

    VkResult Result = VK_SUCCESS;
    
    // TODO: Device layers
    
    // Verify Extensions
    uint32 DeviceExtensionCount = 0;
    Result = vkEnumerateDeviceExtensionProperties(Adapter->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the device extension count");

    TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
    Result = vkEnumerateDeviceExtensionProperties(Adapter->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the device extensions");

    TArray<const char*> EnabledExtensionNames;
    for (const VkExtensionProperties& ExtensionProperty : AvailableDeviceExtensions)
    {
        const char* ExtensionName = ExtensionProperty.extensionName;
        if (DeviceDesc.RequiredExtensionNames.Contains(ExtensionName, RawStringComparator) || DeviceDesc.OptionalExtensionNames.Contains(ExtensionName, RawStringComparator))
        {
            EnabledExtensionNames.Push(ExtensionName);
            ExtensionNames.insert(String(ExtensionName));
        }
    }

    for (const char* ExtensionName : DeviceDesc.RequiredExtensionNames)
    {
        if (!EnabledExtensionNames.Contains(ExtensionName, RawStringComparator))
        {
            VULKAN_ERROR_ALWAYS(String("Instance layer '") + ExtensionName + "' could not be enabled");
            return false;
        }
    }

    // Log enabled extensions and layers
    IConsoleVariable* VerboseVulkan = CConsoleManager::Get().FindVariable("vulkan.VerboseLogging");

    const bool bVerboseLogging = VerboseVulkan && VerboseVulkan->GetBool();
    if (bVerboseLogging)
    {
        if (!EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Device Extensions:");
            
            for (const char* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO(String("    ") + ExtensionName);
            }
        }
    }

    QueueIndicies = CVulkanPhysicalDevice::GetQueueFamilyIndices(Adapter->GetVkPhysicalDevice());
    if (!QueueIndicies)
    {
        VULKAN_ERROR_ALWAYS("Failed to query queue indices");
        return false;
    }
    
    VULKAN_INFO("QueueIndicies: Graphics=" + ToString(QueueIndicies->GraphicsQueueIndex) + ", Compute=" + ToString(QueueIndicies->ComputeQueueIndex) + ", Copy=" + ToString(QueueIndicies->CopyQueueIndex));

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
        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.pNext            = nullptr;
        QueueCreateInfo.flags            = 0;
        QueueCreateInfo.pQueuePriorities = &DefaultQueuePriority;
        QueueCreateInfo.queueFamilyIndex = QueueFamiliy;
        QueueCreateInfo.queueCount       = 1;

        QueueCreateInfos.Push(QueueCreateInfo);
    }
    

    VkDeviceCreateInfo DeviceCreateInfo;
    CMemory::Memzero(&DeviceCreateInfo);

    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.flags                   = 0;
    DeviceCreateInfo.enabledLayerCount       = 0;
    DeviceCreateInfo.ppEnabledLayerNames     = nullptr;
    DeviceCreateInfo.enabledExtensionCount   = EnabledExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = EnabledExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = QueueCreateInfos.Size();
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.Data();

    CVulkanStructureHelper DeviceCreateHelper(DeviceCreateInfo);

    VkPhysicalDeviceFeatures2 DeviceFeatures2;
    CMemory::Memzero(&DeviceFeatures2);

    DeviceFeatures2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    DeviceFeatures2.features = DeviceDesc.RequiredFeatures;
    DeviceCreateHelper.AddNext(DeviceFeatures2);

    // TODO: Check for the availability of these features when the device is created
#if VK_KHR_shader_draw_parameters
    VkPhysicalDeviceShaderDrawParametersFeatures ShaderDrawParametersFeatures;
    CMemory::Memzero(&ShaderDrawParametersFeatures);

    ShaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    
    if (IsExtensionEnabled(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME))
    {
        ShaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;
    }

    DeviceCreateHelper.AddNext(ShaderDrawParametersFeatures);
#endif

#if VK_KHR_buffer_device_address
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR BufferDeviceAddressFeatures;
    CMemory::Memzero(&BufferDeviceAddressFeatures);

    BufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;

    if (IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        BufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    }

    DeviceCreateHelper.AddNext(BufferDeviceAddressFeatures);
#endif

#if VK_KHR_timeline_semaphore
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR TimelineSemaphoreFeatures;
    CMemory::Memzero(&TimelineSemaphoreFeatures);

    TimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;

    if (IsExtensionEnabled(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
    {
        TimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
    }

    DeviceCreateHelper.AddNext(TimelineSemaphoreFeatures);
#endif

    Result = vkCreateDevice(Adapter->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    VULKAN_CHECK_RESULT(Result, "Failed to create Device");
    
    return true;
}

bool CVulkanDevice::AllocateMemory(const VkMemoryAllocateInfo& MemoryAllocationInfo, VkDeviceMemory* OutDeviceMemory)
{
    VULKAN_ERROR(OutDeviceMemory != nullptr, "DeviceMemory cannot be nullptr");
    
    VkResult Result = vkAllocateMemory(Device, &MemoryAllocationInfo, nullptr, OutDeviceMemory);
    VULKAN_CHECK_RESULT(Result, "vkAllocateMemory failed");
    
    NumAllocations++;
    
    const VkPhysicalDeviceProperties& DeviceProperties = Adapter->GetDeviceProperties();
    VULKAN_INFO(String("[AllocateMemory] Allocated=") + ToString(MemoryAllocationInfo.allocationSize) + " Bytes, Allocation = " + ToString(NumAllocations.Load())+ "/" + ToString(DeviceProperties.limits.maxMemoryAllocationCount));
    
    if (NumAllocations.Load() > MemoryAllocationInfo.allocationSize)
    {
        VULKAN_WARNING("Too many allocations");
    }

    return true;
}

void CVulkanDevice::FreeMemory(VkDeviceMemory* OutDeviceMemory)
{
    VULKAN_ERROR(OutDeviceMemory != nullptr, "DeviceMemory cannot be nullptr");
    
    vkFreeMemory(Device, *OutDeviceMemory, nullptr);
    *OutDeviceMemory = VK_NULL_HANDLE;
    
    NumAllocations--;
    
    const VkPhysicalDeviceProperties& DeviceProperties = Adapter->GetDeviceProperties();
    VULKAN_INFO(String("[FreeMemory] Allocation = ") + ToString(NumAllocations.Load()) + "/" + ToString(DeviceProperties.limits.maxMemoryAllocationCount));
}

uint32 CVulkanDevice::GetCommandQueueIndexFromType(EVulkanCommandQueueType Type) const
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
        VULKAN_ERROR_ALWAYS("Invalid CommandQueueType");
        return (~0U);
    }
}
