#pragma once
#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Windows/WindowsApplication.h"
#include "VulkanRHI/Base/VulkanPlatformBase.h"
#include "VulkanRHI/VulkanExtensions.h"

struct VulkanPlatformWindows : public VulkanPlatformBase
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
    #if VK_KHR_win32_surface
        OutInstanceExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true, true));
    #endif
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
        Win32SurfaceCreateInfo.hinstance = GWindowsApplication->GetInstance();

        return vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, OutSurface);
    #else
        return VK_ERROR_UNKNOWN;
    #endif
    }
};
