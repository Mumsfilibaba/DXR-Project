#include "Core/Core.h"
#include "Core/Mac/Mac.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Containers/String.h"

#include <Appkit/Appkit.h>
#include <pthread.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

extern int32 EngineMain(const CHAR* Args[], int32 NumArgs);

static FString GMacCommandLine;
static int32   GEngineMainResult = 0;

@interface FCocoaAppDelegate : NSObject<NSApplicationDelegate>
- (void)runApplicationThread;
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

- (void)runApplicationThread
{
    // Startup the mainloop
    auto CommandLine = GMacCommandLine.GetCString();
    GEngineMainResult = EngineMain(&CommandLine, 1);
    
    if (GEngineMainResult == 0)
    {
        ExecuteOnMainThread(^
        {
            [NSApp terminate:nil];
        }, NSDefaultRunLoopMode, true);
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) Sender
{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*) Notification
{
    NSLog(@"applicationWillTerminate");
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
    SetupAppThread(self, @selector(runApplicationThread));
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
                FString Argument;
                FString ArgumentValue;
                CurrentArg.Split('=', Argument, ArgumentValue);
                CurrentArg = FString::CreateFormatted("%s=\"%s\"", Argument.GetCString(), ArgumentValue.GetCString());
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
    
    ShutdownAppThread();
    return GEngineMainResult;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
