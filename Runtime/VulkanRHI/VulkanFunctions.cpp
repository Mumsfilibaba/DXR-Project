#include "VulkanFunctions.h"
#include "VulkanDriverInstance.h"

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

#if VK_EXT_debug_utils
	VULKAN_FUNCTION_DEFINITION(SetDebugUtilsObjectNameEXT);
	VULKAN_FUNCTION_DEFINITION(CreateDebugUtilsMessengerEXT);
	VULKAN_FUNCTION_DEFINITION(DestroyDebugUtilsMessengerEXT);
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

#if VK_EXT_debug_utils
	if (Instance->IsExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		VULKAN_LOAD_FUNCTION(Instance, SetDebugUtilsObjectNameEXT);
		VULKAN_LOAD_FUNCTION(Instance, CreateDebugUtilsMessengerEXT);
		VULKAN_LOAD_FUNCTION(Instance, DestroyDebugUtilsMessengerEXT);
	}
#endif

	return true;
}
