#include "VulkanLoader.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-Instance Created Functions

VULKAN_FUNCTION_DEFINITION(GetInstanceProcAddr);

VULKAN_FUNCTION_DEFINITION(CreateInstance);
VULKAN_FUNCTION_DEFINITION(DestroyInstance);
VULKAN_FUNCTION_DEFINITION(EnumerateInstanceExtensionProperties);
VULKAN_FUNCTION_DEFINITION(EnumerateInstanceLayerProperties);

#if VK_EXT_debug_utils
    VULKAN_FUNCTION_DEFINITION(SetDebugUtilsObjectNameEXT);
    VULKAN_FUNCTION_DEFINITION(CreateDebugUtilsMessengerEXT);
    VULKAN_FUNCTION_DEFINITION(DestroyDebugUtilsMessengerEXT);
#endif

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Instance Functions

VULKAN_FUNCTION_DEFINITION(EnumeratePhysicalDevices);
VULKAN_FUNCTION_DEFINITION(EnumerateDeviceExtensionProperties);

VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceProperties);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceFeatures);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceMemoryProperties);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceProperties2);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceFeatures2);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceMemoryProperties2);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceQueueFamilyProperties);

VULKAN_FUNCTION_DEFINITION(CreateDevice);
VULKAN_FUNCTION_DEFINITION(DestroyDevice);

VULKAN_FUNCTION_DEFINITION(GetDeviceProcAddr);

#if VK_EXT_metal_surface
    VULKAN_FUNCTION_DEFINITION(CreateMetalSurfaceEXT);
#endif

#if VK_MVK_macos_surface
    VULKAN_FUNCTION_DEFINITION(CreateMacOSSurfaceMVK);
#endif

#if VK_KHR_win32_surface
    VULKAN_FUNCTION_DEFINITION(CreateWin32SurfaceKHR);
#endif

#if VK_KHR_surface
    VULKAN_FUNCTION_DEFINITION(DestroySurfaceKHR);
    
    VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceSurfaceFormatsKHR);
    VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceSurfacePresentModesKHR);
    VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceSurfaceSupportKHR);
#endif

bool LoadInstanceFunctions(CVulkanInstance* Instance)
{
    VULKAN_ERROR(Instance, "Instance cannot be nullptr");

    VkInstance InstanceHandle = Instance->GetVkInstance();
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, EnumeratePhysicalDevices);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, EnumerateDeviceExtensionProperties);

    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceProperties);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceFeatures);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceMemoryProperties);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceProperties2);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceFeatures2);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceMemoryProperties2);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceQueueFamilyProperties);

    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, CreateDevice);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, DestroyDevice);

    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetDeviceProcAddr);

#if VK_EXT_metal_surface
    if (Instance->IsExtensionEnabled(VK_EXT_METAL_SURFACE_EXTENSION_NAME))
    {
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, CreateMetalSurfaceEXT);
    }
#endif
    
#if VK_MVK_macos_surface
    if (Instance->IsExtensionEnabled(VK_MVK_MACOS_SURFACE_EXTENSION_NAME))
    {
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, CreateMacOSSurfaceMVK);
    }
#endif

#if VK_KHR_win32_surface
    if (Instance->IsExtensionEnabled(VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
    {
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, CreateWin32SurfaceKHR);
    }
#endif

#if VK_KHR_surface
    if (Instance->IsExtensionEnabled(VK_KHR_SURFACE_EXTENSION_NAME))
    {
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, DestroySurfaceKHR);

        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceSurfaceCapabilitiesKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceSurfaceFormatsKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceSurfacePresentModesKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceSurfaceSupportKHR);
    }
#endif
    
    return true;
}

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Device Functions

VULKAN_FUNCTION_DEFINITION(DeviceWaitIdle);
VULKAN_FUNCTION_DEFINITION(QueueWaitIdle);

VULKAN_FUNCTION_DEFINITION(CreateCommandPool);
VULKAN_FUNCTION_DEFINITION(ResetCommandPool);
VULKAN_FUNCTION_DEFINITION(DestroyCommandPool);

VULKAN_FUNCTION_DEFINITION(CreateFence);
VULKAN_FUNCTION_DEFINITION(WaitForFences);
VULKAN_FUNCTION_DEFINITION(ResetFences);
VULKAN_FUNCTION_DEFINITION(DestroyFence);

VULKAN_FUNCTION_DEFINITION(CreateSemaphore);
VULKAN_FUNCTION_DEFINITION(DestroySemaphore);

VULKAN_FUNCTION_DEFINITION(CreateImageView);
VULKAN_FUNCTION_DEFINITION(DestroyImageView);

VULKAN_FUNCTION_DEFINITION(AllocateMemory);
VULKAN_FUNCTION_DEFINITION(FreeMemory);

VULKAN_FUNCTION_DEFINITION(CreateBuffer);
VULKAN_FUNCTION_DEFINITION(GetBufferMemoryRequirements);
VULKAN_FUNCTION_DEFINITION(BindBufferMemory);
VULKAN_FUNCTION_DEFINITION(DestroyBuffer);

#if VK_KHR_buffer_device_address
    VULKAN_FUNCTION_DEFINITION(GetBufferDeviceAddressKHR);
#endif

#if VK_KHR_get_memory_requirements2
	VULKAN_FUNCTION_DEFINITION(GetImageMemoryRequirements2KHR);
	VULKAN_FUNCTION_DEFINITION(GetBufferMemoryRequirements2KHR);
	VULKAN_FUNCTION_DEFINITION(GetImageSparseMemoryRequirements2KHR);
#endif

VULKAN_FUNCTION_DEFINITION(AllocateCommandBuffers);
VULKAN_FUNCTION_DEFINITION(FreeCommandBuffers);

VULKAN_FUNCTION_DEFINITION(BeginCommandBuffer);
VULKAN_FUNCTION_DEFINITION(EndCommandBuffer);

VULKAN_FUNCTION_DEFINITION(GetDeviceQueue);
VULKAN_FUNCTION_DEFINITION(QueueSubmit);

#if VK_KHR_swapchain
    VULKAN_FUNCTION_DEFINITION(CreateSwapchainKHR);
    VULKAN_FUNCTION_DEFINITION(DestroySwapchainKHR);

    VULKAN_FUNCTION_DEFINITION(AcquireNextImageKHR);
    VULKAN_FUNCTION_DEFINITION(QueuePresentKHR);

    VULKAN_FUNCTION_DEFINITION(GetSwapchainImagesKHR);
#endif

VULKAN_FUNCTION_DEFINITION(CmdClearColorImage);
VULKAN_FUNCTION_DEFINITION(CmdClearDepthStencilImage);
VULKAN_FUNCTION_DEFINITION(CmdBeginRenderPass);
VULKAN_FUNCTION_DEFINITION(CmdEndRenderPass);
VULKAN_FUNCTION_DEFINITION(CmdPipelineBarrier);

bool LoadDeviceFunctions(CVulkanDevice* Device)
{
    VULKAN_ERROR(Device, "Device cannot be nullptr");

    VkDevice DeviceHandle = Device->GetVkDevice();
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DeviceWaitIdle);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, QueueWaitIdle);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateCommandPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetCommandPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyCommandPool);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateFence);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, WaitForFences);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetFences);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyFence);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateSemaphore);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroySemaphore);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, AllocateMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, FreeMemory);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetBufferMemoryRequirements);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, BindBufferMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyBuffer);

#if VK_KHR_buffer_device_address
    if (Device->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetBufferDeviceAddressKHR);
    }
#endif
	
#if VK_KHR_get_memory_requirements2
	if (Device->IsExtensionEnabled(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME))
	{
		VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetImageMemoryRequirements2KHR);
		VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetBufferMemoryRequirements2KHR);
		VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetImageSparseMemoryRequirements2KHR);
	}
#endif
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateImageView);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyImageView);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, AllocateCommandBuffers);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, FreeCommandBuffers);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, BeginCommandBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, EndCommandBuffer);
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetDeviceQueue);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, QueueSubmit);

#if VK_KHR_swapchain
    if (Device->IsExtensionEnabled(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
    {
        VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateSwapchainKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroySwapchainKHR);

        VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, AcquireNextImageKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, QueuePresentKHR);

        VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetSwapchainImagesKHR);
    }
#endif

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdClearColorImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdClearDepthStencilImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBeginRenderPass);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdEndRenderPass);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdPipelineBarrier);

	// Initialize Dedicated Allocation extension helper
	CVulkanDedicatedAllocationKHR::Initialize(Device);
	
    return true;
}

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDedicatedAllocationKHR

bool CVulkanDedicatedAllocationKHR::bIsEnabled = false;

void CVulkanDedicatedAllocationKHR::Initialize(CVulkanDevice* Device)
{
	VULKAN_ERROR(Device != nullptr, "Device cannot be nullptr");
	
#if (VK_KHR_get_memory_requirements2) && (VK_KHR_dedicated_allocation)
	if (Device->IsExtensionEnabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) && Device->IsExtensionEnabled(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME))
	{
		bIsEnabled = true;
	}
#endif
}
