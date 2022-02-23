#pragma once

#if PLATFORM_WINDOWS
#include "VulkanRHI/Interface/PlatformVulkanMisc.h"

#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Windows/WindowsApplication.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsVulkanExtensions

class CWindowsVulkanExtensions : public CPlatformVulkanExtensions
{
public:

    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions()
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredInstanceLayers()
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceExtensions() 
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, class CPlatformWindow* InWindow, VkSurface* OutSurface)
    {
#if VK_KHR_win32_surface
        CWindowsWindow* WindowsWindow = reinterpret_cast<CWindowsWindow*>(InWindow);

        VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo;
        CMemory::Memzero(&SurfaceCreateInfo);

        SurfaceCreateInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        SurfaceCreateInfo.pNext     = nullptr;
        SurfaceCreateInfo.flags     = 0;
        SurfaceCreateInfo.hwnd      = WindowsWindow->GetHandle();
        SurfaceCreateInfo.hinstance = CWindowsApplication::Get().GetInstance();

        return vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, nullptr, &OutSurface);
#else
        return VK_NULL_HANDLE;
#endif
    }
};

#endif