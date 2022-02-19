#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanFunctions.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

TSharedRef<CVulkanDevice> CVulkanDevice::CreateDevice(CVulkanDriverInstance* InInstance, CVulkanPhysicalDevice* InAdapter, const SVulkanDeviceDesc& DeviceDesc)
{
    VULKAN_ERROR(InInstance != nullptr, "Instance cannot be nullptr");
    VULKAN_ERROR(InAdapter  != nullptr, "Adapter cannot be nullptr");

	TSharedRef<CVulkanDevice> NewDevice = dbg_new CVulkanDevice(InInstance, InAdapter);
    if (NewDevice && NewDevice->Initialize(DeviceDesc))
    {
        return NewDevice;
    }
    else
    {
        return nullptr;
    }
}

CVulkanDevice::CVulkanDevice(CVulkanDriverInstance* InInstance, CVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , Adapter(InAdapter)
    , Device(VK_NULL_HANDLE)
{
}

CVulkanDevice::~CVulkanDevice()
{
    if (VULKAN_CHECK_HANDLE(Device))
    {
        NVulkan::DestroyDevice(Device, nullptr);
    }
}

bool CVulkanDevice::Initialize(const SVulkanDeviceDesc& DeviceDesc)
{
    VkDeviceCreateInfo DeviceCreateInfo;
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.pNext                   = nullptr;
    DeviceCreateInfo.flags                   = 0;
    DeviceCreateInfo.enabledLayerCount       = DeviceDesc.DeviceLayerNames.Size();
    DeviceCreateInfo.ppEnabledLayerNames     = DeviceDesc.DeviceLayerNames.Data();
    DeviceCreateInfo.enabledExtensionCount   = DeviceDesc.DeviceExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = DeviceDesc.DeviceExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = 0;
    DeviceCreateInfo.pQueueCreateInfos       = nullptr;
    DeviceCreateInfo.pEnabledFeatures        = nullptr;

    VkResult Result = NVulkan::CreateDevice(Adapter->GetPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    VULKAN_CHECK_RESULT(Result, "Failed to create Device");
    
    return true;
}
