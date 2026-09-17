#include "Core/Misc/ConsoleManager.h"
#include "Core/Mac/MacThreadManager.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/Mac/VulkanPlatformMac.h"
#include <QuartzCore/QuartzCore.h>

static NSScreen* FindScreenForWindow(FCocoaWindow* CocoaWindow)
{
    if (NSScreen* Screen = CocoaWindow.screen)
    {
        return Screen;
    }

    const NSRect WindowFrame = CocoaWindow.frame;

    NSScreen* BestScreen = nil;
    CGFloat   BestArea   = 0.0;

    for (NSScreen* Screen in [NSScreen screens])
    {
        const NSRect  Intersection = NSIntersectionRect(WindowFrame, Screen.frame);
        const CGFloat Area         = Intersection.size.width * Intersection.size.height;

        if (Area > BestArea)
        {
            BestArea   = Area;
            BestScreen = Screen;
        }
    }

    return BestScreen ? BestScreen : [NSScreen mainScreen];
}

#if VK_KHR_surface
VkResult VulkanPlatformMac::CreateSurface(VkInstance Instance, void* WindowHandle, VkSurfaceKHR* OutSurface)
{
    SCOPED_AUTORELEASE_POOL();

    FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(WindowHandle);

    __block bool bResult;
    __block CAMetalLayer* MetalLayer;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        MetalLayer = [CAMetalLayer layer];
        if (!MetalLayer)
        {
            VULKAN_ERROR_CRITICAL("Failed to create CAMetalLayer");
            bResult = false;
            return;
        }

        if ([CocoaWindow isOpaque])
        {
            [MetalLayer setBackgroundColor:CocoaWindow.backgroundColor.CGColor];
        }
        else
        {
            [MetalLayer setOpaque:NO];
            [MetalLayer setBackgroundColor:nil];
        }

        NSScreen* Screen = FindScreenForWindow(CocoaWindow);
        [MetalLayer setContentsScale:Screen.backingScaleFactor];

        FCocoaWindowView* CocoaWindowView = CocoaWindow.contentView;
        [MetalLayer setFrame:CocoaWindowView.bounds];
        [MetalLayer setAutoresizingMask:kCALayerWidthSizable | kCALayerHeightSizable];
        [MetalLayer setNeedsDisplayOnBoundsChange:YES];
        [MetalLayer setMasksToBounds:YES];

        [MetalLayer setActions:@{
            @"bounds"   : [NSNull null],
            @"position" : [NSNull null],
            @"contents" : [NSNull null],
        }];

        [MetalLayer setContentsGravity:kCAGravityResize];

        [CocoaWindowView setLayer:MetalLayer];
        [CocoaWindowView setWantsLayer:YES];
        [CocoaWindowView setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];

        bResult = true;
    }, NSDefaultRunLoopMode, true);
    
    if (!bResult)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

#if VK_EXT_metal_surface
    VkMetalSurfaceCreateInfoEXT MetalSurfaceCreateInfo;
    Memory::Memzero(&MetalSurfaceCreateInfo);

    MetalSurfaceCreateInfo.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    MetalSurfaceCreateInfo.pNext  = nullptr;
    MetalSurfaceCreateInfo.flags  = 0;
    MetalSurfaceCreateInfo.pLayer = MetalLayer;

    return vkCreateMetalSurfaceEXT(Instance, &MetalSurfaceCreateInfo, nullptr, OutSurface);
#elif VK_MVK_macos_surface
    VkMacOSSurfaceCreateInfoMVK MacOSSurfaceCreateInfo;
    Memory::Memzero(&MacOSSurfaceCreateInfo);

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
