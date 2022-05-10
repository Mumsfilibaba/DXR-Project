#pragma once
#include <AppKit/AppKit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaWindow

@interface CCocoaWindow : NSWindow<NSWindowDelegate>

- (instancetype)init: (CMacApplication*)InApplication ContentRect: (NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

@property (readonly, nonatomic) CMacApplication* Application;

@end
