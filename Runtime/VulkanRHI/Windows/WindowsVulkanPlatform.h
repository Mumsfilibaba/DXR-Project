#pragma once
#include "VulkanCore.h"
#include "VulkanRHI/Generic/GenericVulkanPlatform.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Windows/WindowsApplication.h"

struct FWindowsVulkanPlatform : public FGenericVulkanPlatform
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
        #if VK_KHR_win32_surface
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
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
        return FPlatformLibrary::LoadDynamicLib("vulkan-1");
    }

    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, void* InWindowHandle, VkSurfaceKHR* OutSurface)
    {
    #if VK_KHR_win32_surface
        VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo;
        FMemory::Memzero(&Win32SurfaceCreateInfo);

        Win32SurfaceCreateInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        Win32SurfaceCreateInfo.pNext     = nullptr;
        Win32SurfaceCreateInfo.flags     = 0;
        Win32SurfaceCreateInfo.hwnd      = reinterpret_cast<HWND>(InWindowHandle);
        Win32SurfaceCreateInfo.hinstance = FWindowsApplication::Get()->GetInstance();

        return vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, OutSurface);
    #else
        return VK_ERROR_UNKNOWN;
    #endif
    }
};