#include "MacVulkanPlatform.h"
#include "VulkanLoader.h"
#include "CoreApplication/Mac/CocoaWindow.h"

#include <QuartzCore/QuartzCore.h>

@implementation FMetalWindowView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    CHECK(self != nil);
    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
    return YES;
}

@end

#if VK_KHR_surface
VkResult FMacVulkanPlatform::CreateSurface(VkInstance Instance, void* WindowHandle, VkSurfaceKHR* OutSurface)
{
    SCOPED_AUTORELEASE_POOL();
    
    FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(WindowHandle);
    
    // Create a new MetalWindowView instead of the standard CocoaView (Use the same frame)
    FMetalWindowView* MetalView = [[FMetalWindowView alloc] initWithFrame:CocoaWindow.contentView.frame];
    [MetalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [MetalView setWantsLayer:YES];
    
    // Create a metal layer and set it as the layer on the view
    CAMetalLayer* MetalLayer = [CAMetalLayer new];
    [MetalView setLayer:MetalLayer];

    // Set the MetalView as the new ContentView
    [CocoaWindow setContentView:MetalView];
    [CocoaWindow makeFirstResponder:MetalView];

    // Create the vulkan surface
#if VK_EXT_metal_surface
    VkMetalSurfaceCreateInfoEXT MetalSurfaceCreateInfo;
    FMemory::Memzero(&MetalSurfaceCreateInfo);

    MetalSurfaceCreateInfo.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    MetalSurfaceCreateInfo.pNext  = nullptr;
    MetalSurfaceCreateInfo.flags  = 0;
    MetalSurfaceCreateInfo.pLayer = MetalLayer;

    return vkCreateMetalSurfaceEXT(Instance, &MetalSurfaceCreateInfo, nullptr, OutSurface);
#elif VK_MVK_macos_surface // NOTE: This path is supposed to be deprecated
    VkMacOSSurfaceCreateInfoMVK MacOSSurfaceCreateInfo;
    FMemory::Memzero(&MacOSSurfaceCreateInfo);

    MacOSSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    MacOSSurfaceCreateInfo.pNext = nullptr;
    MacOSSurfaceCreateInfo.flags = 0;
    MacOSSurfaceCreateInfo.pView = ContentView;

    return vkCreateMacOSSurfaceMVK(Instance, &MacOSSurfaceCreateInfo, nullptr, OutSurface);
#else
    return VK_ERROR_UNKNOWN;
#endif
}
#endif