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
        // NOTE: Setting self.delegate = self does not work in here
        [super setOpaque:YES];
        [super setRestorable:NO];
        [super disableSnapshotRestoration];
    }
    
    return self;
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

- (void)performZoom:(id)Sender
{
}

- (void)zoom:(id)Sender
{
    SCOPED_AUTORELEASE_POOL();
    [super zoom:Sender];
}

- (void)keyDown:(NSEvent*)Event
{
}

- (void)keyUp:(NSEvent*)Event
{
}

- (void) performClose:(id)sender
{
}

- (void) windowWillClose:(NSNotification*) Notification
{
    SCOPED_AUTORELEASE_POOL();
    [self setDelegate:nil];
    
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void) windowDidResize:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void) windowDidMove:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void) windowDidMiniaturize:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void) windowDidDeminiaturize:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void) windowDidEnterFullScreen:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void) windowDidExitFullScreen:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidBecomeMain:(NSNotification*) Notification
{
    SCOPED_AUTORELEASE_POOL();
    
    if ([NSApp isHidden] == NO)
    {
        [self orderFront:nil];
    }
 
    // TODO: Do we want to handle key as well?
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidResignMain:(NSNotification*)Notification
{
    SCOPED_AUTORELEASE_POOL();
    
    [self setMovable: YES];
    [self setMovableByWindowBackground: NO];
}

@end
