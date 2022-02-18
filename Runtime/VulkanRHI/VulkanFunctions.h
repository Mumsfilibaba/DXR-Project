#pragma once
#include "VulkanCore.h"

#define VULKAN_FUNCTION_DECLARATION(FunctionName) extern PFN_vk##FunctionName FunctionName
#define VULKAN_FUNCTION_DEFINITION(FunctionName)  PFN_vk##FunctionName FunctionName = nullptr

class CVulkanDriverInstance;

namespace NVulkan
{
    /*//////////////////////////////////////////////////////////////////////////////////////////////*/
    // Global Functions

    VULKAN_FUNCTION_DECLARATION(CreateInstance);
    VULKAN_FUNCTION_DECLARATION(EnumerateInstanceExtensionProperties);
    VULKAN_FUNCTION_DECLARATION(EnumerateInstanceLayerProperties);

    /*//////////////////////////////////////////////////////////////////////////////////////////////*/
    // Instance Functions

    VULKAN_FUNCTION_DECLARATION(EnumeratePhysicalDevices);
    VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceProperties);
    VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceFeatures);
    VULKAN_FUNCTION_DECLARATION(GetPhysicalDeviceQueueFamilyProperties);
    VULKAN_FUNCTION_DECLARATION(CreateDevice);
    VULKAN_FUNCTION_DECLARATION(GetDeviceProcAddr);
    VULKAN_FUNCTION_DECLARATION(DestroyInstance);
    VULKAN_FUNCTION_DECLARATION(DestroyDevice);

    VULKAN_FUNCTION_DECLARATION(SetDebugUtilsObjectNameEXT);
    VULKAN_FUNCTION_DECLARATION(CreateDebugUtilsMessengerEXT);
    VULKAN_FUNCTION_DECLARATION(DestroyDebugUtilsMessengerEXT);

    bool LoadInstanceFunctions(CVulkanDriverInstance* Instance);
}
