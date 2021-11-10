#pragma once

#if PLATFORM_MACOS
#if defined(__OBJC__)

#include <AppKit/AppKit.h>

class CMacApplication;

@interface CCocoaWindow : NSWindow<NSWindowDelegate>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication ContentRect : (NSRect)ContentRect StyleMask : (NSWindowStyleMask)StyleMask Backing : (NSBackingStoreType)BackingStoreType Defer : (BOOL)Flag;
@end

#else

class CCocoaWindow;

#endif
#endif
