#pragma once
#include <AppKit/AppKit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaWindow

@interface CCocoaWindow : NSWindow<NSWindowDelegate>
{
    CMacApplication* Application;
}

- (id)init: (CMacApplication*)InApplication ContentRect: (NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

@end
