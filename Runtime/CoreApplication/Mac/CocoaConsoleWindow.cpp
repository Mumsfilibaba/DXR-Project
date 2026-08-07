#include "Core/Mac/Mac.h"
#include "CoreApplication/Mac/CocoaConsoleWindow.h"
#include "CoreApplication/Mac/MacConsoleOutputDevice.h"

@implementation FCocoaConsoleWindow

- (instancetype) init:(FMacConsoleOutputDevice*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag
{
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:NSBackingStoreBuffered defer:NO];
    if (self)
    {
        ConsoleWindow = InConsoleWindow;
        self.delegate = self;
    }
    
    return self;
}

- (BOOL) acceptsFirstResponder
{
    return NO;
}

- (void) windowWillClose:(NSNotification*) Notification
{
    @autoreleasepool
    {
        [self setDelegate:nil];
    }

    if (ConsoleWindow)
    {
        ConsoleWindow->OnWindowDidClose();
        ConsoleWindow = nullptr;
    }
}

+ (NSString*) convertStringWithArgs:(const CHAR*) Format Args:(va_list)Args
{
    SCOPED_AUTORELEASE_POOL();
    
    NSString* TempFormat = @(Format);
    return [[NSString alloc] initWithFormat:TempFormat arguments:Args];
}

@end
