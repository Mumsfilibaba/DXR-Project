#include "Core/Core.h"
#include "Core/Mac/Mac.h"
#include "Core/Containers/String.h"

#include <Appkit/Appkit.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

extern int32 EngineMain(const CHAR* Args[], int32 NumArgs);

static FString GMacCommandLine;
static int32   GEngineMainResult = 0;

@interface FCocoaAppDelegate : NSObject<NSApplicationDelegate>
@end

@implementation FCocoaAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*) Sender
{
    if (!IsEngineExitRequested())
    {
        return NSTerminateLater;
    }
    else
    {
        return NSTerminateNow;
    }
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
    const CHAR* CommandLine = GMacCommandLine.GetCString();
    GEngineMainResult = EngineMain(&CommandLine, 1);
}

@end


int main(int NumArgs, const CHAR** Args)
{
    // The first argument is always the path to the application
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GMacCommandLine += " ";
        FString CurrentArg(Args[Index]);
        if (CurrentArg.Contains(' '))
        {
            if (CurrentArg.Contains('='))
            {
                int32 Pos       = CurrentArg.FindChar('=');
                int32 Remaining = CurrentArg.Length() - Pos;
                
                FString Arg0(CurrentArg.GetCString(), Pos);
                FString Arg1(CurrentArg.GetCString() + Pos + 1, Remaining);
                CurrentArg = FString::CreateFormatted("%s=\"%s\"", Arg0.GetCString(), Arg1.GetCString());
            }
            else
            {
                CurrentArg = FString::CreateFormatted("\"%s\"", CurrentArg.GetCString());
            }
        }
        
        GMacCommandLine += CurrentArg;
    }
    
    [NSApplication sharedApplication];
    [NSApp setDelegate:[FCocoaAppDelegate new]];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp run];
    return GEngineMainResult;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
