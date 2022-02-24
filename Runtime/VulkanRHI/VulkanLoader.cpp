#include "VulkanLoader.h"
#include "VulkanDriverInstance.h"
#include "VulkanDevice.h"

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Load Functions Helper macro

#define VULKAN_LOAD_FUNCTION(LoaderInstance, FunctionName)                                        \
    do                                                                                            \
    {                                                                                             \
		vk##FunctionName = LoaderInstance->LoadFunction<PFN_vk##FunctionName>("vk"#FunctionName); \
        if (!vk##FunctionName)                                                                    \
        {                                                                                         \
            VULKAN_ERROR_ALWAYS("Failed to load vk"#FunctionName);                                \
            return false;                                                                         \
        }                                                                                         \
    } while(false)

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-Instance Created Functions

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

bool LoadInstanceFunctions(CVulkanDriverInstance* Instance)
{
	VULKAN_ERROR(Instance, "Instance cannot be nullptr");

	VULKAN_LOAD_FUNCTION(Instance, EnumeratePhysicalDevices);
	VULKAN_LOAD_FUNCTION(Instance, EnumerateDeviceExtensionProperties);

	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceProperties);
	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceFeatures);
	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceMemoryProperties);
	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceProperties2);
	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceFeatures2);
	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceMemoryProperties2);
	VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceQueueFamilyProperties);

	VULKAN_LOAD_FUNCTION(Instance, CreateDevice);
	VULKAN_LOAD_FUNCTION(Instance, DestroyDevice);

	VULKAN_LOAD_FUNCTION(Instance, GetDeviceProcAddr);

#if VK_EXT_metal_surface
	if (Instance->IsExtensionEnabled(VK_EXT_METAL_SURFACE_EXTENSION_NAME))
	{
		VULKAN_LOAD_FUNCTION(Instance, CreateMetalSurfaceEXT);
	}
#endif
	
#if VK_MVK_macos_surface
    if (Instance->IsExtensionEnabled(VK_MVK_MACOS_SURFACE_EXTENSION_NAME))
    {
		VULKAN_LOAD_FUNCTION(Instance, CreateMacOSSurfaceMVK);
	}
#endif

#if VK_KHR_win32_surface
	if (Instance->IsExtensionEnabled(VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
	{
		VULKAN_LOAD_FUNCTION(Instance, CreateWin32SurfaceKHR);
	}
#endif

#if VK_KHR_surface
    if (Instance->IsExtensionEnabled(VK_KHR_SURFACE_EXTENSION_NAME))
    {
		VULKAN_LOAD_FUNCTION(Instance, DestroySurfaceKHR);
		VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceSurfaceFormatsKHR);
		VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceSurfacePresentModesKHR);
		VULKAN_LOAD_FUNCTION(Instance, GetPhysicalDeviceSurfaceSupportKHR);
	}
#endif
	
	return true;
}

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Device Functions

VULKAN_FUNCTION_DEFINITION(DeviceWaitIdle);

VULKAN_FUNCTION_DEFINITION(CreateCommandPool);
VULKAN_FUNCTION_DEFINITION(ResetCommandPool);
VULKAN_FUNCTION_DEFINITION(DestroyCommandPool);

VULKAN_FUNCTION_DEFINITION(CreateFence);
VULKAN_FUNCTION_DEFINITION(WaitForFences);
VULKAN_FUNCTION_DEFINITION(ResetFences);
VULKAN_FUNCTION_DEFINITION(DestroyFence);

VULKAN_FUNCTION_DEFINITION(CreateSemaphore);
VULKAN_FUNCTION_DEFINITION(DestroySemaphore);

VULKAN_FUNCTION_DEFINITION(AllocateCommandBuffers);
VULKAN_FUNCTION_DEFINITION(FreeCommandBuffers);

VULKAN_FUNCTION_DEFINITION(BeginCommandBuffer);
VULKAN_FUNCTION_DEFINITION(EndCommandBuffer);

VULKAN_FUNCTION_DEFINITION(GetDeviceQueue);
VULKAN_FUNCTION_DEFINITION(QueueSubmit);

bool LoadDeviceFunctions(CVulkanDevice* Device)
{
	VULKAN_LOAD_FUNCTION(Device, DeviceWaitIdle);

	VULKAN_LOAD_FUNCTION(Device, CreateCommandPool);
	VULKAN_LOAD_FUNCTION(Device, ResetCommandPool);
	VULKAN_LOAD_FUNCTION(Device, DestroyCommandPool);

	VULKAN_LOAD_FUNCTION(Device, CreateFence);
	VULKAN_LOAD_FUNCTION(Device, WaitForFences);
	VULKAN_LOAD_FUNCTION(Device, ResetFences);
	VULKAN_LOAD_FUNCTION(Device, DestroyFence);

	VULKAN_LOAD_FUNCTION(Device, CreateSemaphore);
	VULKAN_LOAD_FUNCTION(Device, DestroySemaphore);	

	VULKAN_LOAD_FUNCTION(Device, AllocateCommandBuffers);
	VULKAN_LOAD_FUNCTION(Device, FreeCommandBuffers);

	VULKAN_LOAD_FUNCTION(Device, BeginCommandBuffer);
	VULKAN_LOAD_FUNCTION(Device, EndCommandBuffer);
	
	VULKAN_LOAD_FUNCTION(Device, GetDeviceQueue);
	VULKAN_LOAD_FUNCTION(Device, QueueSubmit);
	
	return true;
}
