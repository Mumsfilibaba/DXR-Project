#if defined(PLATFORM_MACOS)
#include "Notification.h"
#include "CocoaAppDelegate.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformDebugMisc.h"

@implementation CCocoaAppDelegate

- (id) init:(CMacApplication*) InApplication
{
    self = [super init];
    if (self)
    {
        Application = InApplication;
        Assert( Application != nullptr );
    }

    return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *) Sender
{
    // TODO: Maybe check some state before returning yes, but for now just terminate
    PlatformDebugMisc::OutputDebugString("Should Terminate");
    return NSTerminateNow;
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication* ) Sender
{
    PlatformDebugMisc::OutputDebugString("Terminate After Last Window Closed");
    return YES;
}

- (void) applicationWillTerminate:(NSNotification* ) InNotification
{
    SNotification Notification;
    Notification.Notification = InNotification;
    Application->HandleNotification(Notification);
    
    PlatformDebugMisc::OutputDebugString("Will Terminate");
}

@end

#endif
