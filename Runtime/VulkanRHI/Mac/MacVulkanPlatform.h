#pragma once
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/CocoaWindowView.h"
#include "VulkanRHI/Generic/GenericVulkanPlatform.h"

@interface FMetalWindowView : FCocoaWindowView
@end

struct FMacVulkanPlatform : public FGenericVulkanPlatform
{
    static FORCEINLINE TArray<const CHAR*> GetRequiredInstanceExtensions()
    { 
        return 
        {
        #if VK_KHR_surface
            VK_KHR_SURFACE_EXTENSION_NAME,
        #endif
        #if VK_EXT_debug_utils
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #endif
        #if VK_EXT_metal_surface
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
        #endif
        #if VK_MVK_macos_surface
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
        #endif

        // NOTE: This extension is required since we want to use MoltekVK, otherwise no devices will be reported to exist
        // since MoltenVK does only support a subset of the Vulkan standard.
        #if VK_KHR_portability_enumeration 
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
        };
    }

    static FORCEINLINE TArray<const CHAR*> GetRequiredInstanceLayers()
    { 
        return TArray<const CHAR*>(); 
    }

    static FORCEINLINE TArray<const CHAR*> GetRequiredDeviceExtensions() 
    { 
        return
        {
        #if VK_KHR_swapchain
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        #endif
        };
    }

    static FORCEINLINE TArray<const CHAR*> GetRequiredDeviceLayers()
    { 
        return TArray<const CHAR*>(); 
    }

    static FORCEINLINE void* LoadVulkanLibrary()
    {
        return FPlatformLibrary::LoadDynamicLib("vulkan");
    }

#if VK_KHR_surface
    static VkResult CreateSurface(VkInstance Instance, void* WindowHandle, VkSurfaceKHR* OutSurface);
#endif
};
