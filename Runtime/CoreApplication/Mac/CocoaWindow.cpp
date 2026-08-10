#include "Core/Mac/Mac.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "CoreApplication/Mac/MacApplication.h"
#include "CoreApplication/Mac/MacWindow.h"

@implementation FCocoaWindow

- (instancetype)initWithContentRect:(NSRect)ContentRect styleMask:(NSWindowStyleMask)StyleMask backing:(NSBackingStoreType)BackingStoreType defer:(BOOL)Flag
{
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:BackingStoreType defer:Flag];
    if (self)
    {
        [super disableSnapshotRestoration];
    }
    
    return self;
}

// Allow the window to become the key window (i.e., receive keyboard input)
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (void)keyDown:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)keyUp:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)windowWillClose:(NSNotification*)Notification
{
    @autoreleasepool
    {
        // Remove the window's delegate to prevent further messages
        [self setDelegate:nil];
    }

    if (GMacApplication)
    {
        TSharedRef<FMacWindow> Window = GMacApplication->FindWindowFromNSWindow(self);
        GMacApplication->CloseWindow(Window);
    }
}

- (NSSize)windowWillResize:(NSWindow*)Sender toSize:(NSSize)FrameSize
{
    if (GMacApplication)
    {
        TSharedRef<FMacWindow> Window = GMacApplication->FindWindowFromNSWindow(self);
        GMacApplication->OnWindowWillResize(Window);
    }
    
    return FrameSize;
}

- (void)windowDidResize:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        TSharedRef<FMacWindow> Window = GMacApplication->FindWindowFromNSWindow(self);
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidMove:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidMiniaturize:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidDeminiaturize:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidEnterFullScreen:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidExitFullScreen:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidBecomeMain:(NSNotification*)Notification
{
    @autoreleasepool
    {
        if ([NSApp isHidden] == NO)
        {
            // Order the window to the front of its level to ensure it is visible to the user
            [self orderFront:nil];
        }

        if (GMacApplication)
        {
            GMacApplication->DeferEvent(Notification);
        }
    }
}

- (void)windowDidResignMain:(NSNotification*)Notification
{
    @autoreleasepool
    {
        [self setMovable:YES];
        [self setMovableByWindowBackground:NO];
        
        if (GMacApplication)
        {
            GMacApplication->DeferEvent(Notification);
        }
    }
}

@end

@implementation FCocoaWindowView

- (instancetype)initWithFrame:(NSRect)Frame
{
    self = [super initWithFrame:Frame];
    if (self)
    {
        TrackingArea = nil;
        [self updateTrackingAreas];
        return self;
    }
    
    return nil;
}

- (void)dealloc
{
    if (TrackingArea)
    {
        [self removeTrackingArea:TrackingArea];
        [TrackingArea release];
        TrackingArea = nil;
    }
    
    [super dealloc];
}

- (void)updateTrackingAreas
{
    if (TrackingArea)
    {
        [self removeTrackingArea:TrackingArea];
        [TrackingArea release];
    }
    
    const NSTrackingAreaOptions Options = NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
    TrackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:Options owner:self userInfo:nil];
    [self addTrackingArea:TrackingArea];
    
    [super updateTrackingAreas];
}

- (BOOL)preservesContentDuringLiveResize
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)Event
{
    return YES;
}

- (void)mouseDown:(NSEvent*)Event
{
    @autoreleasepool
    {
        FCocoaWindow* CocoaWindow = [[self window] isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)[self window] : nil;
        if (CocoaWindow)
        {
            [CocoaWindow mouseDown:Event];
        }
        else
        {
            [super mouseDown:Event];
        }
    }
}

- (void)mouseUp:(NSEvent*)Event
{
    @autoreleasepool
    {
        FCocoaWindow* CocoaWindow = [[self window] isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)[self window] : nil;
        if (CocoaWindow)
        {
            [CocoaWindow mouseUp:Event];
        }
        else
        {
            [super mouseUp:Event];
        }
    }
}

- (void)rightMouseDown:(NSEvent*)Event
{
    @autoreleasepool
    {
        FCocoaWindow* CocoaWindow = [[self window] isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)[self window] : nil;
        if (CocoaWindow)
        {
            [CocoaWindow rightMouseDown:Event];
        }
        else
        {
            [super rightMouseDown:Event];
        }
    }
}

- (void)rightMouseUp:(NSEvent*)Event
{
    @autoreleasepool
    {
        FCocoaWindow* CocoaWindow = [[self window] isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)[self window] : nil;
        if (CocoaWindow)
        {
            [CocoaWindow rightMouseUp:Event];
        }
        else
        {
            [super rightMouseUp:Event];
        }
    }
}

- (void)otherMouseDown:(NSEvent*)Event
{
    @autoreleasepool
    {
        FCocoaWindow* CocoaWindow = [[self window] isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)[self window] : nil;
        if (CocoaWindow)
        {
            [CocoaWindow otherMouseDown:Event];
        }
        else
        {
            [super otherMouseDown:Event];
        }
    }
}

- (void)otherMouseUp:(NSEvent*)Event
{
    @autoreleasepool
    {
        FCocoaWindow* CocoaWindow = [[self window] isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)[self window] : nil;
        if (CocoaWindow)
        {
            [CocoaWindow otherMouseUp:Event];
        }
        else
        {
            [super otherMouseUp:Event];
        }
    }
}

- (void)mouseEntered:(NSEvent*)Event
{
    @autoreleasepool
    {
        if (GMacApplication)
        {
            GMacApplication->DeferEvent(Event);
        }
        
        [super mouseEntered:Event];
    }
}

- (void)mouseExited:(NSEvent*)Event
{
    @autoreleasepool
    {
        if (GMacApplication)
        {
            GMacApplication->DeferEvent(Event);
        }
        
        [super mouseExited:Event];
    }
}

@end
