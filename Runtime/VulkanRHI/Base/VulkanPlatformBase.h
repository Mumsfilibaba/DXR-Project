#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/PlatformLibrary.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanExtensions.h"

// Need the beta header for VK_KHR_portability_subset
#include <vulkan/vulkan_beta.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct VulkanPlatformBase
{
    static FORCEINLINE void RetrieveDeviceExtensions(TArray<TUniquePtr<FVulkanDeviceExtension>>& OutDeviceExtensions)       { }
    static FORCEINLINE void RetrieveInstanceExtensions(TArray<TUniquePtr<FVulkanInstanceExtension>>& OutInstanceExtensions) { }

    static FORCEINLINE TArray<const ANSICHAR*> GetRequiredInstanceLayers()
    {
        return TArray<const ANSICHAR*>();
    }

    static FORCEINLINE TArray<const ANSICHAR*> GetOptionalInstanceLayers()
    {
        return TArray<const ANSICHAR*>();
    }

    static FORCEINLINE TArray<const ANSICHAR*> GetRequiredDeviceLayers()
    {
        return TArray<const ANSICHAR*>();
    }

    static FORCEINLINE TArray<const ANSICHAR*> GetOptionalDeviceLayers()
    {
        return TArray<const ANSICHAR*>();
    }

    static FORCEINLINE void* LoadVulkanLibrary() { return nullptr; }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, void* InWindowHandle, VkSurfaceKHR* OutSurface)
    {
        return VK_ERROR_UNKNOWN;
    }
#endif
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
