#include "Core/Core.h"

#include "CoreApplication/Mac/MacApplication.h"

#include <Appkit/Appkit.h>

extern int EngineMain();

class CMacApplication;

static int32 GEngineMainResult = 0;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaAppDelegate

@interface CCocoaAppDelegate : NSObject<NSApplicationDelegate>
@end

@implementation CCocoaAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*) Sender
{
    // TODO: Maybe check some state before returning yes, but for now just terminate
    return NSTerminateNow;
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) Sender
{
    return YES;
}

- (void) applicationWillTerminate:(NSNotification*) Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
    SCOPED_AUTORELEASE_POOL();
    
    // Run the main loop
    GEngineMainResult = EngineMain();
}

@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Main

int main(int NumArgs, const char** Args)
{
    UNREFERENCED_VARIABLE(NumArgs);
    UNREFERENCED_VARIABLE(Args);
    
    [NSApplication sharedApplication];
    [NSApplication sharedApplication].delegate = [[CCocoaAppDelegate alloc] init];;
    
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    [NSApplication sharedApplication].presentationOptions = NSApplicationPresentationDefault;
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    [[NSApplication sharedApplication] run];
    return GEngineMainResult;
}
