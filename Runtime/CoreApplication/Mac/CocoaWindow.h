#pragma once

#if PLATFORM_MACOS
#include <AppKit/AppKit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Cocoa window which is an NSWindow and NSWindowDelegate

@interface CCocoaWindow : NSWindow<NSWindowDelegate>
{
    CMacApplication* Application;
}

- (id)init: (CMacApplication*)InApplication ContentRect: (NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

@end

#endif
