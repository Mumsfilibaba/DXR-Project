#include "VulkanDevice.h"
#include "VulkanFunctions.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

TSharedPtr<CVulkanDevice> CVulkanDevice::CreateDevice(CVulkanDriverInstance* InInstance, const TArray<const char*>& DeviceLayerNames, const TArray<const char*>& DeviceExtensionNames)
{
    VULKAN_ERROR(InInstance != nullptr, "Instance cannot be nullptr");

    TSharedPtr<CVulkanDevice> NewDevice = MakeSharedPtr<CVulkanDevice>(InInstance);
    if (NewDevice && NewDevice->Initialize(DeviceLayerNames, DeviceExtensionNames))
    {
        return NewDevice;
    }
    else
    {
        return nullptr;
    }
}

CVulkanDevice::CVulkanDevice(CVulkanDriverInstance* InInstance)
    : Instance(InInstance)
    , Device(VK_NULL_HANDLE)
    , PhysicalDevice(VK_NULL_HANDLE)
{
}

CVulkanDevice::~CVulkanDevice()
{
    if (VULKAN_CHECK_HANDLE(Device))
    {
        NVulkan::DestroyDevice(Device, nullptr);
    }
}

bool CVulkanDevice::Initialize(const TArray<const char*>& DeviceLayerNames, const TArray<const char*>& DeviceExtensionNames)
{
    VkDeviceCreateInfo DeviceCreateInfo;
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.pNext                   = nullptr;
    DeviceCreateInfo.flags                   = 0;
    DeviceCreateInfo.enabledLayerCount       = DeviceLayerNames.Size();
    DeviceCreateInfo.ppEnabledLayerNames     = DeviceLayerNames.Data();
    DeviceCreateInfo.enabledExtensionCount   = DeviceExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = 0;
    DeviceCreateInfo.pQueueCreateInfos       = nullptr;
    DeviceCreateInfo.pEnabledFeatures        = nullptr;

    VkResult Result = NVulkan::CreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device);
    VULKAN_CHECK(Result, "Failed to create Device");
    
    return true;
}