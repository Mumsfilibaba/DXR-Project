#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDevice.h"

// -------------------------------------------------------------------------------------------
// Function definitions (vkGetInstanceProcAddr + all functions from .inl)
// -------------------------------------------------------------------------------------------

VULKAN_FUNCTION_DEFINITION(GetInstanceProcAddr);

#define VULKAN_GLOBAL_FUNCTION(Name) VULKAN_FUNCTION_DEFINITION(Name);
#define VULKAN_INSTANCE_FUNCTION(Name) VULKAN_FUNCTION_DEFINITION(Name);
#define VULKAN_INSTANCE_FUNCTION_OPTIONAL(Name) VULKAN_FUNCTION_DEFINITION(Name);
#define VULKAN_DEVICE_FUNCTION(Name) VULKAN_FUNCTION_DEFINITION(Name);
#define VULKAN_DEVICE_FUNCTION_OPTIONAL(Name) VULKAN_FUNCTION_DEFINITION(Name);

#include "VulkanRHI/VulkanFunctions.inl"

#undef VULKAN_GLOBAL_FUNCTION
#undef VULKAN_INSTANCE_FUNCTION
#undef VULKAN_INSTANCE_FUNCTION_OPTIONAL
#undef VULKAN_DEVICE_FUNCTION
#undef VULKAN_DEVICE_FUNCTION_OPTIONAL

// -------------------------------------------------------------------------------------------
// Loading
// -------------------------------------------------------------------------------------------

bool VulkanLoader::LoadGlobalFunctions()
{
    #define VULKAN_GLOBAL_FUNCTION(Name)              VULKAN_LOAD_INSTANCE_FUNCTION(VK_NULL_HANDLE, Name);
    #define VULKAN_INSTANCE_FUNCTION(Name)
    #define VULKAN_INSTANCE_FUNCTION_OPTIONAL(Name)
    #define VULKAN_DEVICE_FUNCTION(Name)
    #define VULKAN_DEVICE_FUNCTION_OPTIONAL(Name)
    #include "VulkanRHI/VulkanFunctions.inl"
    #undef VULKAN_GLOBAL_FUNCTION
    #undef VULKAN_INSTANCE_FUNCTION
    #undef VULKAN_INSTANCE_FUNCTION_OPTIONAL
    #undef VULKAN_DEVICE_FUNCTION
    #undef VULKAN_DEVICE_FUNCTION_OPTIONAL

    return true;
}

bool VulkanLoader::LoadInstanceFunctions(FVulkanInstance* Instance)
{
    if (!Instance)
    {
        VULKAN_ERROR_CRITICAL("Instance cannot be nullptr");
        return false;
    }

    VkInstance InstanceHandle = Instance->GetVkInstance();

    #define VULKAN_GLOBAL_FUNCTION(Name)
    #define VULKAN_INSTANCE_FUNCTION(Name) VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, Name);
    #define VULKAN_INSTANCE_FUNCTION_OPTIONAL(Name) VULKAN_TRY_LOAD_INSTANCE_FUNCTION(InstanceHandle, Name);
    #define VULKAN_DEVICE_FUNCTION(Name)
    #define VULKAN_DEVICE_FUNCTION_OPTIONAL(Name)
    
    #include "VulkanRHI/VulkanFunctions.inl"

    #undef VULKAN_GLOBAL_FUNCTION
    #undef VULKAN_INSTANCE_FUNCTION
    #undef VULKAN_INSTANCE_FUNCTION_OPTIONAL
    #undef VULKAN_DEVICE_FUNCTION
    #undef VULKAN_DEVICE_FUNCTION_OPTIONAL

    return true;
}

bool VulkanLoader::LoadDeviceFunctions(FVulkanDevice* Device)
{
    if (!Device)
    {
        VULKAN_ERROR_CRITICAL("Device cannot be nullptr");
        return false;
    }

    VkDevice DeviceHandle = Device->GetVkDevice();

    #define VULKAN_GLOBAL_FUNCTION(Name)
    #define VULKAN_INSTANCE_FUNCTION(Name)
    #define VULKAN_INSTANCE_FUNCTION_OPTIONAL(Name)
    #define VULKAN_DEVICE_FUNCTION(Name) VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, Name);
    #define VULKAN_DEVICE_FUNCTION_OPTIONAL(Name) VULKAN_TRY_LOAD_DEVICE_FUNCTION(DeviceHandle, Name);
    
    #include "VulkanRHI/VulkanFunctions.inl"

    #undef VULKAN_GLOBAL_FUNCTION
    #undef VULKAN_INSTANCE_FUNCTION
    #undef VULKAN_INSTANCE_FUNCTION_OPTIONAL
    #undef VULKAN_DEVICE_FUNCTION
    #undef VULKAN_DEVICE_FUNCTION_OPTIONAL

    return true;
}
