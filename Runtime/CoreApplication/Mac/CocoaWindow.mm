#if PLATFORM_MACOS
#include "CocoaWindow.h"
#include "MacApplication.h"
#include "ScopedAutoreleasePool.h"
#include "Notification.h"

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

- (void) windowWillClose:(NSNotification* ) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidResize:(NSNotification* ) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    const NSRect contentRect = [[self contentView] frame];
    Notification.Size = contentRect.size;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidMove:(NSNotification* ) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    const NSRect contentRect = [self contentRectForFrameRect:[self frame]];
    Notification.Position = contentRect.origin;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidMiniaturize:(NSNotification*) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    const NSRect contentRect = [self contentRectForFrameRect:[self frame]];
    Notification.Size = contentRect.size;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidDeminiaturize:(NSNotification*) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    const NSRect contentRect = [[self contentView] frame];
    Notification.Size = contentRect.size;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidEnterFullScreen:(NSNotification*) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    const NSRect contentRect = [[self contentView] frame];
    Notification.Size = contentRect.size;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidExitFullScreen:(NSNotification*) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    const NSRect contentRect = [[self contentView] frame];
    Notification.Size = contentRect.size;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidBecomeKey:(NSNotification*) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    Application->HandleNotification( Notification );
}

- (void) windowDidResignKey:(NSNotification*) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Notification.Window       = self;
    
    Application->HandleNotification( Notification );
}

@end

#endif
