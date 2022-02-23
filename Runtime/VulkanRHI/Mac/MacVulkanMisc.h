#pragma once

#if PLATFORM_MACOS
#include "CoreApplication/Mac/MacWindow.h"

#include "VulkanRHI/Interface/PlatformVulkanMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacVulkanMisc

class CMacVulkanMisc : public CPlatformVulkanMisc
{
public:

    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions()
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
        };
    }

    static FORCEINLINE TArray<const char*> GetRequiredInstanceLayers()
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceExtensions() 
    { 
        return
		{
#if VK_KHR_swapchain
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#endif
		};
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()
    { 
        return TArray<const char*>(); 
    }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, class CPlatformWindow* InWindow, VkSurfaceKHR* OutSurface)
    {
#if VK_EXT_metal_surface 
        CMacWindow* MacWindow = reinterpret_cast<CMacWindow*>(InWindow);

        VkMetalSurfaceCreateInfoEXT MetalSurfaceCreateInfo;
        CMemory::Memzero(&MetalSurfaceCreateInfo);

        MetalSurfaceCreateInfo.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        MetalSurfaceCreateInfo.pNext  = nullptr;
        MetalSurfaceCreateInfo.flags  = 0;
        MetalSurfaceCreateInfo.pLayer = MacWindow->GetCAMetalLayer();

        return vkCreateMetalSurfaceEXT(Instance, &MetalSurfaceCreateInfo, nullptr, OutSurface);
#elif VK_MVK_macos_surface
		CMacWindow* MacWindow = reinterpret_cast<CMacWindow*>(InWindow);

		VkMacOSSurfaceCreateInfoMVK MacOSSurfaceCreateInfo;
		CMemory::Memzero(&MacOSSurfaceCreateInfo);

		MacOSSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
		MacOSSurfaceCreateInfo.pNext = nullptr;
		MacOSSurfaceCreateInfo.flags = 0;
		MacOSSurfaceCreateInfo.pView = reinterpret_cast<const void*>(MacWindow->GetCocoaContentView());

		return vkCreateMacOSSurfaceMVK(Instance, &MacOSSurfaceCreateInfo, nullptr, OutSurface);
#else
        return VK_ERROR_UNKNOWN;
#endif
    }
#endif
};

#endif
