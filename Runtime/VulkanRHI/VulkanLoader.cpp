#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanExtensions.h"

// -------------------------------------------------------------------------------------------
// Pre-Instance Created Functions
// -------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------
// Instance Functions
// -------------------------------------------------------------------------------------------

VULKAN_FUNCTION_DEFINITION(EnumeratePhysicalDevices);
VULKAN_FUNCTION_DEFINITION(EnumerateDeviceExtensionProperties);

VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceProperties);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceFeatures);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceMemoryProperties);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceProperties2);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceFeatures2);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceMemoryProperties2);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceQueueFamilyProperties);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceFormatProperties);
VULKAN_FUNCTION_DEFINITION(GetPhysicalDeviceImageFormatProperties);

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

// -------------------------------------------------------------------------------------------
// Device Functions
// -------------------------------------------------------------------------------------------

VULKAN_FUNCTION_DEFINITION(DeviceWaitIdle);
VULKAN_FUNCTION_DEFINITION(QueueWaitIdle);

VULKAN_FUNCTION_DEFINITION(CreateCommandPool);
VULKAN_FUNCTION_DEFINITION(ResetCommandPool);
VULKAN_FUNCTION_DEFINITION(DestroyCommandPool);

VULKAN_FUNCTION_DEFINITION(CreateFence);
VULKAN_FUNCTION_DEFINITION(DestroyFence);
VULKAN_FUNCTION_DEFINITION(WaitForFences);
VULKAN_FUNCTION_DEFINITION(ResetFences);
VULKAN_FUNCTION_DEFINITION(GetFenceStatus);

VULKAN_FUNCTION_DEFINITION(CreateSemaphore);
VULKAN_FUNCTION_DEFINITION(DestroySemaphore);
VULKAN_FUNCTION_DEFINITION(WaitSemaphores);
VULKAN_FUNCTION_DEFINITION(GetSemaphoreCounterValue);

VULKAN_FUNCTION_DEFINITION(CreateImageView);
VULKAN_FUNCTION_DEFINITION(DestroyImageView);

VULKAN_FUNCTION_DEFINITION(AllocateMemory);
VULKAN_FUNCTION_DEFINITION(FreeMemory);
VULKAN_FUNCTION_DEFINITION(MapMemory);
VULKAN_FUNCTION_DEFINITION(UnmapMemory);
VULKAN_FUNCTION_DEFINITION(FlushMappedMemoryRanges);
VULKAN_FUNCTION_DEFINITION(InvalidateMappedMemoryRanges);

VULKAN_FUNCTION_DEFINITION(CreateBuffer);
VULKAN_FUNCTION_DEFINITION(GetBufferMemoryRequirements);
VULKAN_FUNCTION_DEFINITION(BindBufferMemory);
VULKAN_FUNCTION_DEFINITION(DestroyBuffer);

// VK_KHR_buffer_device_address (Core in 1.2)
VULKAN_FUNCTION_DEFINITION(GetBufferDeviceAddress);

VULKAN_FUNCTION_DEFINITION(CreateImage);
VULKAN_FUNCTION_DEFINITION(GetImageMemoryRequirements);
VULKAN_FUNCTION_DEFINITION(BindImageMemory);
VULKAN_FUNCTION_DEFINITION(DestroyImage);

// VK_KHR_get_memory_requirements2 (Core in 1.1)
VULKAN_FUNCTION_DEFINITION(GetImageMemoryRequirements2);
VULKAN_FUNCTION_DEFINITION(GetBufferMemoryRequirements2);
VULKAN_FUNCTION_DEFINITION(GetImageSparseMemoryRequirements2);

// VK_KHR_maintenance4 (Core in 1.3)
VULKAN_FUNCTION_DEFINITION(GetDeviceBufferMemoryRequirements);
VULKAN_FUNCTION_DEFINITION(GetDeviceImageMemoryRequirements);

VULKAN_FUNCTION_DEFINITION(CreateShaderModule);
VULKAN_FUNCTION_DEFINITION(DestroyShaderModule);

VULKAN_FUNCTION_DEFINITION(CreateGraphicsPipelines);
VULKAN_FUNCTION_DEFINITION(CreateComputePipelines);
VULKAN_FUNCTION_DEFINITION(DestroyPipeline);

VULKAN_FUNCTION_DEFINITION(CreatePipelineCache);
VULKAN_FUNCTION_DEFINITION(DestroyPipelineCache);
VULKAN_FUNCTION_DEFINITION(GetPipelineCacheData);

VULKAN_FUNCTION_DEFINITION(CreatePipelineLayout);
VULKAN_FUNCTION_DEFINITION(DestroyPipelineLayout);

VULKAN_FUNCTION_DEFINITION(CreateDescriptorSetLayout);
VULKAN_FUNCTION_DEFINITION(DestroyDescriptorSetLayout);

VULKAN_FUNCTION_DEFINITION(CreateDescriptorPool);
VULKAN_FUNCTION_DEFINITION(DestroyDescriptorPool);
VULKAN_FUNCTION_DEFINITION(ResetDescriptorPool);

VULKAN_FUNCTION_DEFINITION(AllocateDescriptorSets);
VULKAN_FUNCTION_DEFINITION(FreeDescriptorSets);
VULKAN_FUNCTION_DEFINITION(UpdateDescriptorSets);

VULKAN_FUNCTION_DEFINITION(CreateSampler);
VULKAN_FUNCTION_DEFINITION(DestroySampler);

VULKAN_FUNCTION_DEFINITION(CreateBufferView);
VULKAN_FUNCTION_DEFINITION(DestroyBufferView);

#if VK_KHR_acceleration_structure
VULKAN_FUNCTION_DEFINITION(CreateAccelerationStructureKHR);
VULKAN_FUNCTION_DEFINITION(DestroyAccelerationStructureKHR);
VULKAN_FUNCTION_DEFINITION(GetAccelerationStructureBuildSizesKHR);
VULKAN_FUNCTION_DEFINITION(GetAccelerationStructureDeviceAddressKHR);
#endif

VULKAN_FUNCTION_DEFINITION(CreateQueryPool);
VULKAN_FUNCTION_DEFINITION(DestroyQueryPool);
VULKAN_FUNCTION_DEFINITION(ResetQueryPool);
VULKAN_FUNCTION_DEFINITION(GetQueryPoolResults);

VULKAN_FUNCTION_DEFINITION(CreateRenderPass);
VULKAN_FUNCTION_DEFINITION(DestroyRenderPass);

VULKAN_FUNCTION_DEFINITION(CreateFramebuffer);
VULKAN_FUNCTION_DEFINITION(DestroyFramebuffer);

VULKAN_FUNCTION_DEFINITION(AllocateCommandBuffers);
VULKAN_FUNCTION_DEFINITION(ResetCommandBuffer);
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
VULKAN_FUNCTION_DEFINITION(CmdResolveImage);
VULKAN_FUNCTION_DEFINITION(CmdBeginRenderPass);
VULKAN_FUNCTION_DEFINITION(CmdEndRenderPass);
VULKAN_FUNCTION_DEFINITION(CmdBeginRendering);
VULKAN_FUNCTION_DEFINITION(CmdEndRendering);
VULKAN_FUNCTION_DEFINITION(CmdSetViewport);
VULKAN_FUNCTION_DEFINITION(CmdSetScissor);
VULKAN_FUNCTION_DEFINITION(CmdSetBlendConstants);
VULKAN_FUNCTION_DEFINITION(CmdBindVertexBuffers);
VULKAN_FUNCTION_DEFINITION(CmdBindIndexBuffer);
VULKAN_FUNCTION_DEFINITION(CmdBindPipeline);
VULKAN_FUNCTION_DEFINITION(CmdBindDescriptorSets);
VULKAN_FUNCTION_DEFINITION(CmdPushConstants);
VULKAN_FUNCTION_DEFINITION(CmdPipelineBarrier);
VULKAN_FUNCTION_DEFINITION(CmdFillBuffer);
VULKAN_FUNCTION_DEFINITION(CmdCopyBuffer);
VULKAN_FUNCTION_DEFINITION(CmdCopyBufferToImage);
VULKAN_FUNCTION_DEFINITION(CmdCopyImageToBuffer);
VULKAN_FUNCTION_DEFINITION(CmdCopyImage);
VULKAN_FUNCTION_DEFINITION(CmdBlitImage);
VULKAN_FUNCTION_DEFINITION(CmdDispatch);
VULKAN_FUNCTION_DEFINITION(CmdDraw);
VULKAN_FUNCTION_DEFINITION(CmdDrawIndexed);
VULKAN_FUNCTION_DEFINITION(CmdWriteTimestamp);
VULKAN_FUNCTION_DEFINITION(CmdBeginQuery);
VULKAN_FUNCTION_DEFINITION(CmdEndQuery);
#if VK_EXT_debug_utils
VULKAN_FUNCTION_DEFINITION(CmdInsertDebugUtilsLabelEXT);
VULKAN_FUNCTION_DEFINITION(CmdBeginDebugUtilsLabelEXT);
VULKAN_FUNCTION_DEFINITION(CmdEndDebugUtilsLabelEXT);
#endif
#if VK_KHR_acceleration_structure
VULKAN_FUNCTION_DEFINITION(CmdBuildAccelerationStructuresKHR);
#endif
VULKAN_FUNCTION_DEFINITION(CmdPipelineBarrier2);
#if VK_AMD_buffer_marker
VULKAN_FUNCTION_DEFINITION(CmdWriteBufferMarkerAMD);
#endif
#if VK_NV_device_diagnostic_checkpoints
VULKAN_FUNCTION_DEFINITION(CmdSetCheckpointNV);
VULKAN_FUNCTION_DEFINITION(GetQueueCheckpointDataNV);
#endif
#if VK_EXT_device_fault
VULKAN_FUNCTION_DEFINITION(GetDeviceFaultInfoEXT);
#endif

bool VulkanLoader::LoadInstanceFunctions(FVulkanInstance* Instance, FVulkanExtensionRegistry& Registry)
{
    if (!Instance)
    {
        VULKAN_ERROR_CRITICAL("Instance cannot be nullptr");
        return false;
    }

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
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceFormatProperties);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetPhysicalDeviceImageFormatProperties);
    
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, CreateDevice);
    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, DestroyDevice);

    VULKAN_LOAD_INSTANCE_FUNCTION(InstanceHandle, GetDeviceProcAddr);

    return Registry.LoadInstanceFunctions(Instance);
}

bool VulkanLoader::LoadDeviceFunctions(FVulkanDevice* Device, FVulkanExtensionRegistry& Registry)
{
    if (!Device)
    {
        VULKAN_ERROR_CRITICAL("Device cannot be nullptr");
        return false;
    }

    VkDevice DeviceHandle = Device->GetVkDevice();
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DeviceWaitIdle);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, QueueWaitIdle);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateCommandPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetCommandPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyCommandPool);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateFence);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyFence);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, WaitForFences);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetFences);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetFenceStatus);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateSemaphore);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroySemaphore);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, WaitSemaphores);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetSemaphoreCounterValue);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, AllocateMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, FreeMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, MapMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, UnmapMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, FlushMappedMemoryRanges);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, InvalidateMappedMemoryRanges);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetBufferMemoryRequirements);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, BindBufferMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyBuffer);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetBufferDeviceAddress);
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetImageMemoryRequirements);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, BindImageMemory);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyImage);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetImageMemoryRequirements2);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetBufferMemoryRequirements2);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetImageSparseMemoryRequirements2);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetDeviceBufferMemoryRequirements);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetDeviceImageMemoryRequirements);
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateShaderModule);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyShaderModule);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateGraphicsPipelines);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyPipeline);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreatePipelineLayout);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateComputePipelines);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyPipelineLayout);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreatePipelineCache);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyPipelineCache);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetPipelineCacheData);
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateDescriptorSetLayout);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyDescriptorSetLayout);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateDescriptorPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyDescriptorPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetDescriptorPool);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, AllocateDescriptorSets);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, FreeDescriptorSets);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, UpdateDescriptorSets);
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateSampler);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroySampler);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateBufferView);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyBufferView);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateQueryPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyQueryPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetQueryPool);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetQueryPoolResults);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateRenderPass);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyRenderPass);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateFramebuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyFramebuffer);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CreateImageView);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, DestroyImageView);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, AllocateCommandBuffers);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, ResetCommandBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, FreeCommandBuffers);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, BeginCommandBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, EndCommandBuffer);
    
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, GetDeviceQueue);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, QueueSubmit);

    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdPipelineBarrier2);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdClearColorImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdClearDepthStencilImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdResolveImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBeginRenderPass);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdEndRenderPass);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBeginRendering);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdEndRendering);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdSetViewport);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdSetScissor);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdSetBlendConstants);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBindVertexBuffers);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBindIndexBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBindPipeline);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBindDescriptorSets);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdPushConstants);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdPipelineBarrier);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdFillBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdCopyBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdCopyBufferToImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdCopyImageToBuffer);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdCopyImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBlitImage);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdDispatch);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdDraw);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdDrawIndexed);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdWriteTimestamp);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdBeginQuery);
    VULKAN_LOAD_DEVICE_FUNCTION(DeviceHandle, CmdEndQuery);

    return Registry.LoadDeviceFunctions(Device);
}

