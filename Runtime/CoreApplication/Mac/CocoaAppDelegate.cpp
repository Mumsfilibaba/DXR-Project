#if PLATFORM_MACOS
#include "Notification.h"
#include "CocoaAppDelegate.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformDebugMisc.h"

@implementation CCocoaAppDelegate

- (id) init:(CMacApplication*) InApplication
{
    Assert( InApplication != nullptr );
    
    self = [super init];
    if (self)
    {
        Application = InApplication;
    }

    return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication* ) Sender
{
    // TODO: Maybe check some state before returning yes, but for now just terminate
    return NSTerminateNow;
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication* ) Sender
{
    return YES;
}

- (void) applicationWillTerminate:(NSNotification* ) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Application->HandleNotification(Notification);
}

@end

#endif