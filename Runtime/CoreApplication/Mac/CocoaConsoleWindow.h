#pragma once 
#include "CoreApplication/Generic/GenericConsoleWindow.h"

#include <stdarg.h>

#include <AppKit/AppKit.h>

class FMacConsoleWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaConsoleWindow

@interface CCocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
	FMacConsoleWindow* ConsoleWindow;
}

// Instance
- (instancetype)init:(FMacConsoleWindow*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

// Static
+(NSString*)convertStringWithArgs:(const char*)Format Args : (va_list)Args;

@end
