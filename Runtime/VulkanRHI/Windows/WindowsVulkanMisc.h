#pragma once
#include "VulkanCore.h"

#if PLATFORM_WINDOWS
#include "VulkanRHI/Interface/PlatformVulkanMisc.h"

#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Windows/WindowsApplication.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsVulkanMisc

class CWindowsVulkanMisc : public CPlatformVulkanMisc
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
#if VK_KHR_win32_surface
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
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

    static FORCEINLINE DynamicLibraryHandle LoadVulkanLibrary()
    {
        return PlatformLibrary::LoadDynamicLib("vulkan-1");
    }

    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, class CPlatformWindow* InWindow, VkSurfaceKHR* OutSurface)
    {
#if VK_KHR_win32_surface
        CWindowsWindow* WindowsWindow = reinterpret_cast<CWindowsWindow*>(InWindow);

        VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo;
        CMemory::Memzero(&Win32SurfaceCreateInfo);

        Win32SurfaceCreateInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        Win32SurfaceCreateInfo.pNext     = nullptr;
        Win32SurfaceCreateInfo.flags     = 0;
        Win32SurfaceCreateInfo.hwnd      = WindowsWindow->GetHandle();
        Win32SurfaceCreateInfo.hinstance = CWindowsApplication::Get()->GetInstance();

        return vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, OutSurface);
#else
        return VK_ERROR_UNKNOWN;
#endif
    }
};

#endif