#pragma once
#include "VulkanCore.h"

#include "Core/Containers/Array.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformVulkan

class CPlatformVulkan
{
public:

    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions() { return TArray<const char*>(); }
    static FORCEINLINE TArray<const char*> GetRequiredInstanceLayers()     { return TArray<const char*>(); }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceExtensions() { return TArray<const char*>(); }
    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()     { return TArray<const char*>(); }

    static FORCEINLINE DynamicLibraryHandle LoadVulkanLibrary() { return 0; }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, void* InWindowHandle, VkSurfaceKHR* OutSurface) { return VK_ERROR_UNKNOWN; }
#endif
};
