#pragma once
#include "VulkanCore.h"

#include "Core/Containers/Array.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformVulkanMisc

class CPlatformVulkanMisc
{
public:

    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions() { return TArray<const char*>(); }
    static FORCEINLINE TArray<const char*> GetRequiredInstanceLayers()     { return TArray<const char*>(); }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceExtensions() { return TArray<const char*>(); }
    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()     { return TArray<const char*>(); }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, class CPlatformWindow* InWindow, VkSurfaceKHR* OutSurface) { return VK_ERROR_UNKNOWN; }
#endif
};
