#pragma once 
#include "CoreApplication/Generic/GenericConsoleWindow.h"

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
- (instancetype)init:(CMacConsoleWindow*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

// Static
+(NSString*)convertStringWithArgs:(const char*)Format Args : (va_list)Args;

@end
