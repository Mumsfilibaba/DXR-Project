#pragma once
#include "VulkanCore.h"

#define VULKAN_FUNCTION_DECLARATION(FunctionName) extern PFN_vk##FunctionName vk##FunctionName
#define VULKAN_FUNCTION_DEFINITION(FunctionName)  PFN_vk##FunctionName vk##FunctionName = nullptr

class CVulkanDriverInstance;
class CVulkanDevice;

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-Instance Created Functions

VULKAN_FUNCTION_DECLARATION(CreateInstance);
VULKAN_FUNCTION_DECLARATION(DestroyInstance);
VULKAN_FUNCTION_DECLARATION(EnumerateInstanceExtensionProperties);
VULKAN_FUNCTION_DECLARATION(EnumerateInstanceLayerProperties);

#if VK_EXT_debug_utils
	VULKAN_FUNCTION_DECLARATION(SetDebugUtilsObjectNameEXT);
	VULKAN_FUNCTION_DECLARATION(CreateDebugUtilsMessengerEXT);
	VULKAN_FUNCTION_DECLARATION(DestroyDebugUtilsMessengerEXT);
#endif

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Instance Functions

VULKAN_FUNCTION_DECLARATION(EnumeratePhysicalDevices);
VULKAN_FUNCTION_DECLARATION(EnumerateDeviceExtensionProperties);

VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceProperties);
VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceFeatures);
VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceMemoryProperties);
VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceProperties2);
VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceFeatures2);
VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceMemoryProperties2);
VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceQueueFamilyProperties);

VULKAN_FUNCTION_DECLARATION(CreateDevice);
VULKAN_FUNCTION_DECLARATION(DestroyDevice);

VULKAN_FUNCTION_DECLARATION(GetDeviceProcAddr);

#if VK_EXT_metal_surface
	VULKAN_FUNCTION_DECLARATION(CreateMetalSurfaceEXT);
#endif

#if VK_MVK_macos_surface
	VULKAN_FUNCTION_DECLARATION(CreateMacOSSurfaceMVK);
#endif

#if VK_KHR_win32_surface
    VULKAN_FUNCTION_DECLARATION(CreateWin32SurfaceKHR);
#endif

#if VK_KHR_surface
	VULKAN_FUNCTION_DECLARATION(DestroySurfaceKHR);
	VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceSurfaceCapabilitiesKHR);
	VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceSurfaceFormatsKHR);
	VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceSurfacePresentModesKHR);
	VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceSurfaceSupportKHR);
#endif

bool LoadInstanceFunctions(CVulkanDriverInstance* Instance);

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Device Functions

VULKAN_FUNCTION_DECLARATION(DeviceWaitIdle);

VULKAN_FUNCTION_DECLARATION(CreateCommandPool);
VULKAN_FUNCTION_DECLARATION(ResetCommandPool);
VULKAN_FUNCTION_DECLARATION(DestroyCommandPool);

VULKAN_FUNCTION_DECLARATION(CreateFence);
VULKAN_FUNCTION_DECLARATION(WaitForFences);
VULKAN_FUNCTION_DECLARATION(ResetFences);
VULKAN_FUNCTION_DECLARATION(DestroyFence);

VULKAN_FUNCTION_DECLARATION(CreateSemaphore);
VULKAN_FUNCTION_DECLARATION(DestroySemaphore);

VULKAN_FUNCTION_DECLARATION(AllocateCommandBuffers);
VULKAN_FUNCTION_DECLARATION(FreeCommandBuffers);

VULKAN_FUNCTION_DECLARATION(BeginCommandBuffer);
VULKAN_FUNCTION_DECLARATION(EndCommandBuffer);

VULKAN_FUNCTION_DECLARATION(GetDeviceQueue);
VULKAN_FUNCTION_DECLARATION(QueueSubmit);

bool LoadDeviceFunctions(CVulkanDevice* Device);
