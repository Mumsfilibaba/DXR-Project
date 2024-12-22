#pragma once
#include <stdarg.h>
#include <AppKit/AppKit.h>

class FMacConsoleOutputDevice;

@interface FCocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
    FMacConsoleOutputDevice* ConsoleWindow;
}

- (instancetype)init:(FMacConsoleOutputDevice*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

+ (NSString*)convertStringWithArgs:(const CHAR*)Format Args : (va_list)Args;

@end
