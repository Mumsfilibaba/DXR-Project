#pragma once 
#include "Core/Misc/OutputDeviceConsole.h"

#include <stdarg.h>

#include <AppKit/AppKit.h>

class FMacOutputDeviceConsole;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCocoaConsoleWindow

@interface FCocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
	FMacOutputDeviceConsole* ConsoleWindow;
}

// Instance
- (instancetype)init:(FMacOutputDeviceConsole*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

// Static
+(NSString*)convertStringWithArgs:(const CHAR*)Format Args : (va_list)Args;

@end
