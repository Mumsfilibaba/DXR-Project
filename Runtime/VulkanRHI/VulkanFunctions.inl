// Single source of truth for all Vulkan function pointers.
// Included multiple times with different macro definitions for declaration, definition, and loading.
//
// Categories:
//   VULKAN_GLOBAL_FUNCTION            - Pre-instance functions (loaded via vkGetInstanceProcAddr(NULL))
//   VULKAN_INSTANCE_FUNCTION          - Core instance functions (must succeed)
//   VULKAN_INSTANCE_FUNCTION_OPTIONAL - Extension instance functions (null is acceptable)
//   VULKAN_DEVICE_FUNCTION            - Core device functions (must succeed)
//   VULKAN_DEVICE_FUNCTION_OPTIONAL   - Extension device functions (null is acceptable)

// -------------------------------------------------------------------------------------------
// Global Functions (Loaded before VkInstance creation)
// -------------------------------------------------------------------------------------------

VULKAN_GLOBAL_FUNCTION(CreateInstance)
VULKAN_GLOBAL_FUNCTION(EnumerateInstanceExtensionProperties)
VULKAN_GLOBAL_FUNCTION(EnumerateInstanceLayerProperties)

// -------------------------------------------------------------------------------------------
// Instance Functions
// -------------------------------------------------------------------------------------------

VULKAN_INSTANCE_FUNCTION(DestroyInstance)
VULKAN_INSTANCE_FUNCTION(EnumeratePhysicalDevices)
VULKAN_INSTANCE_FUNCTION(EnumerateDeviceExtensionProperties)
VULKAN_INSTANCE_FUNCTION(EnumerateDeviceLayerProperties)

VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceProperties)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceFeatures)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceMemoryProperties)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceProperties2)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceFeatures2)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceMemoryProperties2)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceQueueFamilyProperties)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceFormatProperties)
VULKAN_INSTANCE_FUNCTION(GetPhysicalDeviceImageFormatProperties)

VULKAN_INSTANCE_FUNCTION(CreateDevice)
VULKAN_INSTANCE_FUNCTION(DestroyDevice)

VULKAN_INSTANCE_FUNCTION(GetDeviceProcAddr)

// -------------------------------------------------------------------------------------------
// Instance Extension Functions
// -------------------------------------------------------------------------------------------

#if VK_EXT_debug_utils
VULKAN_INSTANCE_FUNCTION_OPTIONAL(SetDebugUtilsObjectNameEXT)
VULKAN_INSTANCE_FUNCTION_OPTIONAL(CreateDebugUtilsMessengerEXT)
VULKAN_INSTANCE_FUNCTION_OPTIONAL(DestroyDebugUtilsMessengerEXT)
#endif

#if VK_EXT_metal_surface
VULKAN_INSTANCE_FUNCTION_OPTIONAL(CreateMetalSurfaceEXT)
#endif

#if VK_MVK_macos_surface
VULKAN_INSTANCE_FUNCTION_OPTIONAL(CreateMacOSSurfaceMVK)
#endif

#if VK_KHR_win32_surface
VULKAN_INSTANCE_FUNCTION_OPTIONAL(CreateWin32SurfaceKHR)
#endif

#if VK_KHR_surface
VULKAN_INSTANCE_FUNCTION_OPTIONAL(DestroySurfaceKHR)
VULKAN_INSTANCE_FUNCTION_OPTIONAL(GetPhysicalDeviceSurfaceCapabilitiesKHR)
VULKAN_INSTANCE_FUNCTION_OPTIONAL(GetPhysicalDeviceSurfaceFormatsKHR)
VULKAN_INSTANCE_FUNCTION_OPTIONAL(GetPhysicalDeviceSurfacePresentModesKHR)
VULKAN_INSTANCE_FUNCTION_OPTIONAL(GetPhysicalDeviceSurfaceSupportKHR)
#endif

// -------------------------------------------------------------------------------------------
// Device Functions
// -------------------------------------------------------------------------------------------

VULKAN_DEVICE_FUNCTION(DeviceWaitIdle)
VULKAN_DEVICE_FUNCTION(QueueWaitIdle)

VULKAN_DEVICE_FUNCTION(CreateCommandPool)
VULKAN_DEVICE_FUNCTION(ResetCommandPool)
VULKAN_DEVICE_FUNCTION(DestroyCommandPool)

VULKAN_DEVICE_FUNCTION(CreateFence)
VULKAN_DEVICE_FUNCTION(DestroyFence)
VULKAN_DEVICE_FUNCTION(WaitForFences)
VULKAN_DEVICE_FUNCTION(ResetFences)
VULKAN_DEVICE_FUNCTION(GetFenceStatus)

VULKAN_DEVICE_FUNCTION(CreateSemaphore)
VULKAN_DEVICE_FUNCTION(DestroySemaphore)
VULKAN_DEVICE_FUNCTION(WaitSemaphores)
VULKAN_DEVICE_FUNCTION(GetSemaphoreCounterValue)

VULKAN_DEVICE_FUNCTION(CreateImageView)
VULKAN_DEVICE_FUNCTION(DestroyImageView)

VULKAN_DEVICE_FUNCTION(AllocateMemory)
VULKAN_DEVICE_FUNCTION(FreeMemory)
VULKAN_DEVICE_FUNCTION(MapMemory)
VULKAN_DEVICE_FUNCTION(UnmapMemory)
VULKAN_DEVICE_FUNCTION(FlushMappedMemoryRanges)
VULKAN_DEVICE_FUNCTION(InvalidateMappedMemoryRanges)

VULKAN_DEVICE_FUNCTION(CreateBuffer)
VULKAN_DEVICE_FUNCTION(GetBufferMemoryRequirements)
VULKAN_DEVICE_FUNCTION(BindBufferMemory)
VULKAN_DEVICE_FUNCTION(DestroyBuffer)

VULKAN_DEVICE_FUNCTION(GetBufferDeviceAddress)

VULKAN_DEVICE_FUNCTION(CreateImage)
VULKAN_DEVICE_FUNCTION(GetImageMemoryRequirements)
VULKAN_DEVICE_FUNCTION(BindImageMemory)
VULKAN_DEVICE_FUNCTION(DestroyImage)

VULKAN_DEVICE_FUNCTION(GetImageMemoryRequirements2)
VULKAN_DEVICE_FUNCTION(GetBufferMemoryRequirements2)
VULKAN_DEVICE_FUNCTION(GetImageSparseMemoryRequirements2)

VULKAN_DEVICE_FUNCTION(GetDeviceBufferMemoryRequirements)
VULKAN_DEVICE_FUNCTION(GetDeviceImageMemoryRequirements)

VULKAN_DEVICE_FUNCTION(CreateShaderModule)
VULKAN_DEVICE_FUNCTION(DestroyShaderModule)

VULKAN_DEVICE_FUNCTION(CreateGraphicsPipelines)
VULKAN_DEVICE_FUNCTION(CreateComputePipelines)
VULKAN_DEVICE_FUNCTION(DestroyPipeline)

VULKAN_DEVICE_FUNCTION(CreatePipelineCache)
VULKAN_DEVICE_FUNCTION(DestroyPipelineCache)
VULKAN_DEVICE_FUNCTION(GetPipelineCacheData)

VULKAN_DEVICE_FUNCTION(CreatePipelineLayout)
VULKAN_DEVICE_FUNCTION(DestroyPipelineLayout)

VULKAN_DEVICE_FUNCTION(CreateDescriptorSetLayout)
VULKAN_DEVICE_FUNCTION(DestroyDescriptorSetLayout)

VULKAN_DEVICE_FUNCTION(CreateDescriptorPool)
VULKAN_DEVICE_FUNCTION(DestroyDescriptorPool)
VULKAN_DEVICE_FUNCTION(ResetDescriptorPool)

VULKAN_DEVICE_FUNCTION(AllocateDescriptorSets)
VULKAN_DEVICE_FUNCTION(FreeDescriptorSets)
VULKAN_DEVICE_FUNCTION(UpdateDescriptorSets)

VULKAN_DEVICE_FUNCTION(CreateRenderPass)
VULKAN_DEVICE_FUNCTION(DestroyRenderPass)

VULKAN_DEVICE_FUNCTION(CreateFramebuffer)
VULKAN_DEVICE_FUNCTION(DestroyFramebuffer)

VULKAN_DEVICE_FUNCTION(CreateSampler)
VULKAN_DEVICE_FUNCTION(DestroySampler)

VULKAN_DEVICE_FUNCTION(CreateBufferView)
VULKAN_DEVICE_FUNCTION(DestroyBufferView)

VULKAN_DEVICE_FUNCTION(CreateQueryPool)
VULKAN_DEVICE_FUNCTION(DestroyQueryPool)
VULKAN_DEVICE_FUNCTION(ResetQueryPool)
VULKAN_DEVICE_FUNCTION(GetQueryPoolResults)

VULKAN_DEVICE_FUNCTION(AllocateCommandBuffers)
VULKAN_DEVICE_FUNCTION(ResetCommandBuffer)
VULKAN_DEVICE_FUNCTION(FreeCommandBuffers)

VULKAN_DEVICE_FUNCTION(BeginCommandBuffer)
VULKAN_DEVICE_FUNCTION(EndCommandBuffer)

VULKAN_DEVICE_FUNCTION(GetDeviceQueue)
VULKAN_DEVICE_FUNCTION(QueueSubmit)

VULKAN_DEVICE_FUNCTION(CmdClearColorImage)
VULKAN_DEVICE_FUNCTION(CmdClearDepthStencilImage)
VULKAN_DEVICE_FUNCTION(CmdResolveImage)
VULKAN_DEVICE_FUNCTION(CmdBeginRenderPass)
VULKAN_DEVICE_FUNCTION(CmdEndRenderPass)
VULKAN_DEVICE_FUNCTION(CmdBeginRendering)
VULKAN_DEVICE_FUNCTION(CmdEndRendering)
VULKAN_DEVICE_FUNCTION(CmdSetViewport)
VULKAN_DEVICE_FUNCTION(CmdSetScissor)
VULKAN_DEVICE_FUNCTION(CmdSetBlendConstants)
VULKAN_DEVICE_FUNCTION(CmdSetStencilReference)
VULKAN_DEVICE_FUNCTION(CmdSetDepthBias)
VULKAN_DEVICE_FUNCTION(CmdBindVertexBuffers)
VULKAN_DEVICE_FUNCTION(CmdBindIndexBuffer)
VULKAN_DEVICE_FUNCTION(CmdBindPipeline)
VULKAN_DEVICE_FUNCTION(CmdBindDescriptorSets)
VULKAN_DEVICE_FUNCTION(CmdPushConstants)
VULKAN_DEVICE_FUNCTION(CmdPipelineBarrier)
VULKAN_DEVICE_FUNCTION(CmdPipelineBarrier2)
VULKAN_DEVICE_FUNCTION(CmdFillBuffer)
VULKAN_DEVICE_FUNCTION(CmdCopyBuffer)
VULKAN_DEVICE_FUNCTION(CmdCopyBufferToImage)
VULKAN_DEVICE_FUNCTION(CmdCopyImageToBuffer)
VULKAN_DEVICE_FUNCTION(CmdCopyImage)
VULKAN_DEVICE_FUNCTION(CmdBlitImage)
VULKAN_DEVICE_FUNCTION(CmdDispatch)
VULKAN_DEVICE_FUNCTION(CmdDraw)
VULKAN_DEVICE_FUNCTION(CmdDrawIndexed)
VULKAN_DEVICE_FUNCTION(CmdWriteTimestamp)
VULKAN_DEVICE_FUNCTION(CmdBeginQuery)
VULKAN_DEVICE_FUNCTION(CmdEndQuery)
VULKAN_DEVICE_FUNCTION(CmdCopyQueryPoolResults)

// -------------------------------------------------------------------------------------------
// Device Extension Functions
// -------------------------------------------------------------------------------------------

#if VK_KHR_swapchain
VULKAN_DEVICE_FUNCTION_OPTIONAL(CreateSwapchainKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(DestroySwapchainKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(AcquireNextImageKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(QueuePresentKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetSwapchainImagesKHR)
#endif

#if VK_KHR_acceleration_structure
VULKAN_DEVICE_FUNCTION_OPTIONAL(CreateAccelerationStructureKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(DestroyAccelerationStructureKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetAccelerationStructureBuildSizesKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetAccelerationStructureDeviceAddressKHR)
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdBuildAccelerationStructuresKHR)
#endif

#if VK_EXT_debug_utils
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdInsertDebugUtilsLabelEXT)
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdBeginDebugUtilsLabelEXT)
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdEndDebugUtilsLabelEXT)
#endif

#if VK_AMD_buffer_marker
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdWriteBufferMarkerAMD)
#endif

#if VK_NV_device_diagnostic_checkpoints
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdSetCheckpointNV)
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetQueueCheckpointDataNV)
#endif

#if VK_EXT_transform_feedback
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdBindTransformFeedbackBuffersEXT)
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdBeginTransformFeedbackEXT)
VULKAN_DEVICE_FUNCTION_OPTIONAL(CmdEndTransformFeedbackEXT)
#endif

#if VK_EXT_device_fault
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetDeviceFaultInfoEXT)
#endif

#if VK_KHR_calibrated_timestamps
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetCalibratedTimestampsKHR)
#elif VK_EXT_calibrated_timestamps
VULKAN_DEVICE_FUNCTION_OPTIONAL(GetCalibratedTimestampsEXT)
#endif
