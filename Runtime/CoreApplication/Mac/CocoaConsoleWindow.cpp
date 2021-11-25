#if PLATFORM_MACOS && defined(__OBJC__)
#include "CocoaConsoleWindow.h"
#include "MacConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

@implementation CCocoaConsoleWindow

- (id) init:(CMacConsoleWindow*)InConsoleWindow ContentRect:(NSRect)ContentRect StyleMask: (NSWindowStyleMask)StyleMask Backing: (NSBackingStoreType)BackingStoreType Defer: (BOOL)Flag
{
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:NSBackingStoreBuffered defer:NO];
    if ( self )
    {
		ConsoleWindow = InConsoleWindow;
		[self setDelegate:self];
    }
    
    return self;
}

- (BOOL) windowShouldClose:(NSWindow*) Sender
{
    SCOPED_AUTORELEASE_POOL();
    
    [Sender release];
    return YES;
}

- (BOOL) acceptsFirstResponder
{
    return NO;
}

- (void) windowWillClose:(NSNotification*) Notification
{
	ConsoleWindow->OnWindowDidClose();
}

+ (NSString*) convertStringWithArgs:(const char*) Format Args:(va_list)Args
{
    NSString* TempFormat = [NSString stringWithUTF8String:Format];
    return [[NSString alloc] initWithFormat:TempFormat arguments:Args];
}

@end

#endif
