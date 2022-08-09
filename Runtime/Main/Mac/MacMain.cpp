#include "Core/Core.h"
#include "Core/Mac/Mac.h"

#include <Appkit/Appkit.h>

extern int GenericMain();

class FMacApplication;

static int32 GEngineMainResult = 0;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCocoaAppDelegate

@interface FCocoaAppDelegate : NSObject<NSApplicationDelegate>
@end

@implementation FCocoaAppDelegate

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
    NSLog(@"applicationWillTerminate");
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
    SCOPED_AUTORELEASE_POOL();
    
    // Run the main loop
    GEngineMainResult = GenericMain();
}

@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Main

int main(int NumArgs, const char** Args)
{
    UNREFERENCED_VARIABLE(NumArgs);
    UNREFERENCED_VARIABLE(Args);
    
    [NSApplication sharedApplication];
    [NSApplication sharedApplication].delegate = [FCocoaAppDelegate new];
    
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    [NSApplication sharedApplication].presentationOptions = NSApplicationPresentationDefault;
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    [[NSApplication sharedApplication] run];
    return GEngineMainResult;
}
