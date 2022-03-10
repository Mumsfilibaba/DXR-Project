#pragma once 

#if PLATFORM_MACOS
#include "CoreApplication/Interface/PlatformConsoleWindow.h"

#include <stdarg.h>

#include <AppKit/AppKit.h>

class CMacConsoleWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaConsoleWindow

@interface CCocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
    CMacConsoleWindow* ConsoleWindow;
}

// Instance
- (id)init:(CMacConsoleWindow*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

// Static
+(NSString*)convertStringWithArgs:(const char*)Format Args : (va_list)Args;

@end

#endif
