#pragma once
#include <stdarg.h>
#include <AppKit/AppKit.h>

class FMacConsoleWindow;

@interface FCocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
    FMacConsoleWindow* ConsoleWindow;
}

- (instancetype)init:(FMacConsoleWindow*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag;

+ (NSString*)convertStringWithArgs:(const CHAR*)Format Args : (va_list)Args;

@end
