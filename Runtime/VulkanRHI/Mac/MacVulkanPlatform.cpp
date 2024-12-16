#include "MacVulkanPlatform.h"
#include "VulkanLoader.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Mac/MacThreadManager.h"
#include "CoreApplication/Mac/CocoaWindow.h"

#include <QuartzCore/QuartzCore.h>

#if VK_KHR_surface
VkResult FMacVulkanPlatform::CreateSurface(VkInstance Instance, void* WindowHandle, VkSurfaceKHR* OutSurface)
{
    SCOPED_AUTORELEASE_POOL();
    
    FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(WindowHandle);

    // Set the MetalView as the new ContentView
    __block bool bResult;
    __block CAMetalLayer* MetalLayer;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        // Create a metal layer and set it as the layer on the view
        MetalLayer = [CAMetalLayer layer];
        if (!MetalLayer)
        {
            VULKAN_ERROR("Failed to create CAMetalLayer");
            bResult = false;
            return;
        }
        
        // Set BackgroundColor to black
        CGColorRef BackgroundColor = CGColorGetConstantColor(kCGColorBlack);
        [MetalLayer setBackgroundColor:BackgroundColor];
        [MetalLayer setContentsScale:1.0f];

        // Create a new MetalWindowView instead of the standard CocoaView (Use the same frame)
        FCocoaWindowView* CocoaWindowView = CocoaWindow.contentView;
        [CocoaWindowView setLayer:MetalLayer];
        [CocoaWindowView setWantsLayer:YES];
        
        bResult = true;
    }, NSDefaultRunLoopMode, true);
    
    if (!bResult)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Create the vulkan surface
#if VK_EXT_metal_surface
    VkMetalSurfaceCreateInfoEXT MetalSurfaceCreateInfo;
    FMemory::Memzero(&MetalSurfaceCreateInfo);

    MetalSurfaceCreateInfo.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    MetalSurfaceCreateInfo.pNext  = nullptr;
    MetalSurfaceCreateInfo.flags  = 0;
    MetalSurfaceCreateInfo.pLayer = MetalLayer;

    return vkCreateMetalSurfaceEXT(Instance, &MetalSurfaceCreateInfo, nullptr, OutSurface);
#elif VK_MVK_macos_surface
    VkMacOSSurfaceCreateInfoMVK MacOSSurfaceCreateInfo;
    FMemory::Memzero(&MacOSSurfaceCreateInfo);

    MacOSSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    MacOSSurfaceCreateInfo.pNext = nullptr;
    MacOSSurfaceCreateInfo.flags = 0;
    MacOSSurfaceCreateInfo.pView = MetalLayer;

    return vkCreateMacOSSurfaceMVK(Instance, &MacOSSurfaceCreateInfo, nullptr, OutSurface);
#else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
#endif
}
#endif
