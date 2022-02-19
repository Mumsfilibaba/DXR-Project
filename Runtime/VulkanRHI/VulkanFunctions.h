#pragma once
#include "VulkanCore.h"

#define VULKAN_FUNCTION_DECLARATION(FunctionName) extern PFN_vk##FunctionName vk##FunctionName
#define VULKAN_FUNCTION_DEFINITION(FunctionName)  PFN_vk##FunctionName vk##FunctionName = nullptr

class CVulkanDriverInstance;

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

bool LoadInstanceFunctions(CVulkanDriverInstance* Instance);
