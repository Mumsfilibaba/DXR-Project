#include "CocoaWindow.h"
#include "MacApplication.h"

#include "Core/Mac/Mac.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaWindow

@implementation CCocoaWindow

- (instancetype) initWithContentRect:(NSRect)ContentRect StyleMask:(NSWindowStyleMask)StyleMask Backing:(NSBackingStoreType)BackingStoreType Defer:(BOOL)Flag
{
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:BackingStoreType defer:Flag];
    if (self)
    {
        self.delegate = self;
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
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidResize:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidMove:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidMiniaturize:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidDeminiaturize:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidEnterFullScreen:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidExitFullScreen:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidBecomeKey:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void) windowDidResignKey:(NSNotification*) InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

@end
