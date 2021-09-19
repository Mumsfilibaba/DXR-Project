#if defined(PLATFORM_MACOS)
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Application/Platform/PlatformDebugMisc.h"
#include "Notification.h"

#include "Core/Application/Mac/CocoaAppDelegate.h"

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
