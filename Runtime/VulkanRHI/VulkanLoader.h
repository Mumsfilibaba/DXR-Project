#pragma once
#include "VulkanRHI/VulkanCore.h"

// -------------------------------------------------------------------------------------------
// Loader macros
// -------------------------------------------------------------------------------------------

#define VULKAN_FUNCTION_DECLARATION(FunctionName) extern PFN_vk##FunctionName vk##FunctionName
#define VULKAN_FUNCTION_DEFINITION(FunctionName)  PFN_vk##FunctionName vk##FunctionName = nullptr

#define VULKAN_LOAD_INSTANCE_FUNCTION(Instance, FunctionName) \
    do \
    { \
        vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetInstanceProcAddr(Instance, "vk"#FunctionName)); \
        if (!vk##FunctionName) \
        { \
            VULKAN_ERROR_CRITICAL("Failed to load vk"#FunctionName); \
            return false; \
        } \
    } while(false)

#define VULKAN_LOAD_DEVICE_FUNCTION(Device, FunctionName) \
    do \
    { \
        vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetDeviceProcAddr(Device, "vk"#FunctionName)); \
        if (!vk##FunctionName) \
        { \
            VULKAN_ERROR_CRITICAL("Failed to load vk"#FunctionName); \
            return false; \
        } \
    } while(false)

#define VULKAN_LOAD_DEVICE_FUNCTION_ALIAS(Device, FunctionName, FallbackName) \
    do \
    { \
        vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetDeviceProcAddr(Device, "vk"#FunctionName)); \
        if (!vk##FunctionName) \
        { \
            vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetDeviceProcAddr(Device, "vk"#FallbackName)); \
        } \
        if (!vk##FunctionName) \
        { \
            VULKAN_ERROR_CRITICAL("Failed to load vk"#FunctionName" (or vk"#FallbackName")"); \
            return false; \
        } \
    } while(false)

#define VULKAN_TRY_LOAD_DEVICE_FUNCTION_ALIAS(Device, FunctionName, FallbackName) \
    do \
    { \
        vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetDeviceProcAddr(Device, "vk"#FunctionName)); \
        if (!vk##FunctionName) \
        { \
            vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetDeviceProcAddr(Device, "vk"#FallbackName)); \
        } \
    } while(false)

#define VULKAN_TRY_LOAD_INSTANCE_FUNCTION(Instance, FunctionName) \
    vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>( \
        vkGetInstanceProcAddr(Instance, "vk"#FunctionName))

#define VULKAN_TRY_LOAD_DEVICE_FUNCTION(Device, FunctionName) \
    vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>( \
        vkGetDeviceProcAddr(Device, "vk"#FunctionName))

// -------------------------------------------------------------------------------------------
// Function declarations (vkGetInstanceProcAddr loaded from library, all others via .inl)
// -------------------------------------------------------------------------------------------

VULKAN_FUNCTION_DECLARATION(GetInstanceProcAddr);

#define VULKAN_GLOBAL_FUNCTION(Name) VULKAN_FUNCTION_DECLARATION(Name);
#define VULKAN_INSTANCE_FUNCTION(Name) VULKAN_FUNCTION_DECLARATION(Name);
#define VULKAN_INSTANCE_FUNCTION_OPTIONAL(Name) VULKAN_FUNCTION_DECLARATION(Name);
#define VULKAN_DEVICE_FUNCTION(Name) VULKAN_FUNCTION_DECLARATION(Name);
#define VULKAN_DEVICE_FUNCTION_OPTIONAL(Name) VULKAN_FUNCTION_DECLARATION(Name);
#define VULKAN_DEVICE_FUNCTION_ALIAS(Name, FallbackName) VULKAN_FUNCTION_DECLARATION(Name);
#define VULKAN_DEVICE_FUNCTION_ALIAS_OPTIONAL(Name, FallbackName) VULKAN_FUNCTION_DECLARATION(Name);

#include "VulkanRHI/VulkanFunctions.inl"

#undef VULKAN_GLOBAL_FUNCTION
#undef VULKAN_INSTANCE_FUNCTION
#undef VULKAN_INSTANCE_FUNCTION_OPTIONAL
#undef VULKAN_DEVICE_FUNCTION
#undef VULKAN_DEVICE_FUNCTION_OPTIONAL
#undef VULKAN_DEVICE_FUNCTION_ALIAS
#undef VULKAN_DEVICE_FUNCTION_ALIAS_OPTIONAL

// -------------------------------------------------------------------------------------------
// Loader
// -------------------------------------------------------------------------------------------

class FVulkanDevice;
class FVulkanInstance;

struct VulkanLoader
{
    static bool LoadGlobalFunctions();
    static bool LoadInstanceFunctions(FVulkanInstance* Instance);
    static bool LoadDeviceFunctions(FVulkanDevice* Device);
};
