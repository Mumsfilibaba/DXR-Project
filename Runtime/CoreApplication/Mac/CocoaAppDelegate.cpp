#include "CocoaAppDelegate.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformDebugMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaAppDelegate

@implementation CCocoaAppDelegate

- (id) init:(CMacApplication*) InApplication
{
    Check(InApplication != nullptr);
    
    self = [super init];
    if (self)
    {
        Application = InApplication;
    }

    return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*) Sender
{
    // TODO: Maybe check some state before returning yes, but for now just terminate
    return NSTerminateNow;
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) Sender
{
    return YES;
}

- (void) applicationWillTerminate:(NSNotification*) InNotification
{
    Application->DeferEvent(InNotification);
}

@end
