#pragma once
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "VulkanRHI/Base/VulkanPlatformBase.h"
#include "VulkanRHI/VulkanExtensions.h"

struct VulkanPlatformMac : public VulkanPlatformBase
{
    static void RetrieveDeviceExtensions(TArray<TUniquePtr<FVulkanDeviceExtension>>& OutDeviceExtensions)
    {
    #if VK_KHR_swapchain
        OutDeviceExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true, true));
    #endif
    }

    static void RetrieveInstanceExtensions(TArray<TUniquePtr<FVulkanInstanceExtension>>& OutInstanceExtensions)
    {
    #if VK_KHR_surface
        OutInstanceExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_KHR_SURFACE_EXTENSION_NAME, true, true));
    #endif
    #if VK_EXT_metal_surface
        OutInstanceExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_EXT_METAL_SURFACE_EXTENSION_NAME, true, true));
    #endif
    #if VK_MVK_macos_surface
        OutInstanceExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, true, true));
    #endif
    #if VK_KHR_portability_enumeration
        OutInstanceExtensions.Add(MakeUniquePtr<FVulkanKHRPortabilityEnumerationExtension>(true, true));
    #endif
    }

    static FORCEINLINE void* LoadVulkanLibrary()
    {
        return FPlatformLibrary::LoadDynamicLib("vulkan");
    }

#if VK_KHR_surface
    static VkResult CreateSurface(VkInstance Instance, void* WindowHandle, VkSurfaceKHR* OutSurface);
#endif
};
