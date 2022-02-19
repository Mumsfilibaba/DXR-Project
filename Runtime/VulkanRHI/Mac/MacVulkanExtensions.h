#pragma once

#if PLATFORM_MACOS
#include "VulkanRHI/Interface/PlatformVulkanExtensions.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacVulkanExtensions

class CMacVulkanExtensions : public CPlatformVulkanExtensions
{
public:

    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions()
    { 
        return 
        {
            VK_KHR_SURFACE_EXTENSION_NAME, 
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
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
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()
    { 
        return TArray<const char*>(); 
    }
};

#endif
