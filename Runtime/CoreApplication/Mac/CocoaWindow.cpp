#include "CocoaWindow.h"
#include "MacApplication.h"
#include "MacWindow.h"
#include "Core/Mac/Mac.h"

@implementation FCocoaWindow

- (instancetype) initWithContentRect:(NSRect)ContentRect StyleMask:(NSWindowStyleMask)StyleMask Backing:(NSBackingStoreType)BackingStoreType Defer:(BOOL)Flag
{
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:BackingStoreType defer:Flag];
    if (self)
    {
        CachedWidth  = ContentRect.size.width;
        CachedHeight = ContentRect.size.height;
        
        // NOTE: Setting self.delegate = self does not work in here
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

- (void) windowWillClose:(NSNotification*) Notification
{
    @autoreleasepool
    {
        [self setDelegate:nil];
        
        if (MacApplication)
        {
            TSharedRef<FMacWindow> Window = MacApplication->GetWindowFromNSWindow(self);
            MacApplication->CloseWindow(Window);
        }
    }
}

- (void) windowDidResize:(NSNotification*) Notification
{
    const NSRect ContentRect = [self contentRectForFrameRect:self.frame];

    const CGFloat Width  = ContentRect.size.width;
    const CGFloat Height = ContentRect.size.height;
    if (Width != CachedWidth || Height != CachedHeight)
    {
        // Width and height have not changed, send the event
        if (MacApplication)
        {
            MacApplication->DeferEvent(Notification);
        }
    
        // Width or height has changed, update cached values
        CachedWidth  = Width;
        CachedHeight = Height;
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
    @autoreleasepool
    {
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
}

- (void)windowDidResignMain:(NSNotification*)Notification
{
    @autoreleasepool
    {
        [self setMovable: YES];
        [self setMovableByWindowBackground: NO];
    }
}

@end
