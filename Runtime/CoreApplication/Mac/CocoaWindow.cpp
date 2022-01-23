#if PLATFORM_MACOS
#include "CocoaWindow.h"
#include "MacApplication.h"
#include "ScopedAutoreleasePool.h"

@implementation CCocoaWindow

- (id) init:(CMacApplication*) InApplication ContentRect:(NSRect)ContentRect StyleMask:(NSWindowStyleMask)StyleMask Backing:(NSBackingStoreType)BackingStoreType Defer:(BOOL)Flag
{
    Assert(InApplication != nullptr);

    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:BackingStoreType defer:Flag];
    if (self)
    {
        Application = InApplication;

        [self setDelegate:self];
        [self setOpaque:YES];
    }
    
    return self;
}

- (void) dealloc
{
    [super dealloc];
}

- (BOOL) canBecomeKeyWindow
{
    return YES;
}

- (BOOL) canBecomeMainWindow
{
    return YES;
}

- (BOOL) acceptsMouseMovedEvents
{
    return YES;
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) windowWillClose:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidResize:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidMove:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidMiniaturize:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidDeminiaturize:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidEnterFullScreen:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidExitFullScreen:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidBecomeKey:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

- (void) windowDidResignKey:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

@end

#endif
