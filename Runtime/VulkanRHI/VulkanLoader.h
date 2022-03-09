#pragma once
#include "VulkanCore.h"

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Loader macros

#define VULKAN_FUNCTION_DECLARATION(FunctionName) extern PFN_vk##FunctionName vk##FunctionName
#define VULKAN_FUNCTION_DEFINITION(FunctionName)  PFN_vk##FunctionName vk##FunctionName = nullptr

#define VULKAN_LOAD_DEVICE_FUNCTION(Device, FunctionName)                                                          \
    do                                                                                                             \
    {                                                                                                              \
        vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetDeviceProcAddr(Device, "vk"#FunctionName)); \
        if (!vk##FunctionName)                                                                                     \
        {                                                                                                          \
            VULKAN_ERROR_ALWAYS("Failed to load vk"#FunctionName);                                                 \
            return false;                                                                                          \
        }                                                                                                          \
    } while(false)

#define VULKAN_LOAD_INSTANCE_FUNCTION(Instance, FunctionName)                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(vkGetInstanceProcAddr(Instance, "vk"#FunctionName)); \
        if (!vk##FunctionName)                                                                                         \
        {                                                                                                              \
            VULKAN_ERROR_ALWAYS("Failed to load vk"#FunctionName);                                                     \
            return false;                                                                                              \
        }                                                                                                              \
    } while(false)

class CVulkanDriverInstance;
class CVulkanDevice;

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-Instance Created Functions

VULKAN_FUNCTION_DECLARATION(GetInstanceProcAddr);

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
VULKAN_FUNCTION_DECLARATION(QueueWaitIdle);

VULKAN_FUNCTION_DECLARATION(CreateCommandPool);
VULKAN_FUNCTION_DECLARATION(ResetCommandPool);
VULKAN_FUNCTION_DECLARATION(DestroyCommandPool);

VULKAN_FUNCTION_DECLARATION(CreateFence);
VULKAN_FUNCTION_DECLARATION(WaitForFences);
VULKAN_FUNCTION_DECLARATION(ResetFences);
VULKAN_FUNCTION_DECLARATION(DestroyFence);

VULKAN_FUNCTION_DECLARATION(CreateSemaphore);
VULKAN_FUNCTION_DECLARATION(DestroySemaphore);

VULKAN_FUNCTION_DECLARATION(CreateImageView);
VULKAN_FUNCTION_DECLARATION(DestroyImageView);

VULKAN_FUNCTION_DECLARATION(AllocateMemory);
VULKAN_FUNCTION_DECLARATION(FreeMemory);

VULKAN_FUNCTION_DECLARATION(CreateBuffer);
VULKAN_FUNCTION_DECLARATION(GetBufferMemoryRequirements);
VULKAN_FUNCTION_DECLARATION(BindBufferMemory);
VULKAN_FUNCTION_DECLARATION(DestroyBuffer);

#if VK_KHR_buffer_device_address
    VULKAN_FUNCTION_DECLARATION(GetBufferDeviceAddressKHR);
#endif

VULKAN_FUNCTION_DECLARATION(AllocateCommandBuffers);
VULKAN_FUNCTION_DECLARATION(FreeCommandBuffers);

VULKAN_FUNCTION_DECLARATION(BeginCommandBuffer);
VULKAN_FUNCTION_DECLARATION(EndCommandBuffer);

VULKAN_FUNCTION_DECLARATION(GetDeviceQueue);
VULKAN_FUNCTION_DECLARATION(QueueSubmit);

#if VK_KHR_swapchain
    VULKAN_FUNCTION_DECLARATION(CreateSwapchainKHR);
    VULKAN_FUNCTION_DECLARATION(DestroySwapchainKHR);

    VULKAN_FUNCTION_DECLARATION(AcquireNextImageKHR);
    VULKAN_FUNCTION_DECLARATION(QueuePresentKHR);

    VULKAN_FUNCTION_DECLARATION(GetSwapchainImagesKHR);
#endif

VULKAN_FUNCTION_DECLARATION(CmdClearColorImage);
VULKAN_FUNCTION_DECLARATION(CmdClearDepthStencilImage);
VULKAN_FUNCTION_DECLARATION(CmdBeginRenderPass);
VULKAN_FUNCTION_DECLARATION(CmdEndRenderPass);
VULKAN_FUNCTION_DECLARATION(CmdPipelineBarrier);

bool LoadDeviceFunctions(CVulkanDevice* Device);

/*//////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDebugUtilsEXT

class CVulkanDebugUtilsEXT
{
public:
    template<typename HandleType>
    static FORCEINLINE VkResult SetObjectName(VkDevice Device, const char* Name, HandleType ObjectHandle, VkObjectType ObjectType)
    {
        return SetObjectName(Device, Name, reinterpret_cast<uint64>(ObjectHandle), ObjectType);
    }

    static FORCEINLINE VkResult SetObjectName(VkDevice Device, const char* Name, uint64 ObjectHandle, VkObjectType ObjectType)
    {
#if VK_EXT_debug_utils
        VkDebugUtilsObjectNameInfoEXT DebugUtilsObjectNameInfo;
        CMemory::Memzero(&DebugUtilsObjectNameInfo);

        DebugUtilsObjectNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugUtilsObjectNameInfo.pNext        = nullptr;
        DebugUtilsObjectNameInfo.pObjectName  = Name;
        DebugUtilsObjectNameInfo.objectHandle = ObjectHandle;
        DebugUtilsObjectNameInfo.objectType   = ObjectType;

        return vkSetDebugUtilsObjectNameEXT(Device, &DebugUtilsObjectNameInfo);
#endif
    }
};
