#include <Core/Mac/Mac.h>
#include <Core/Mac/MacThreadManager.h>
#include <Core/CoreGlobals.h>
#include <Core/Containers/String.h>
#include <AppKit/AppKit.h>

#include "PlaygroundLoop.h"

// The same shape as Runtime/Launch/Mac/MacMain.cpp: NSApp owns the main thread, and the playground
// runs on the app thread beside it. The Launch module is not linked here, because linking it would
// bring FEngineLoop and the whole engine with it, which is the one thing the playground must not boot.

DISABLE_UNREFERENCED_VARIABLE_WARNING

static String GMacCommandLine;
static int32  GPlaygroundResult = 0;

@interface FPlaygroundAppDelegate : NSObject<NSApplicationDelegate>

- (void)runAppThread;

@end

@implementation FPlaygroundAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*) Sender
{
    if (!IsEngineExitRequested())
    {
        RequestEngineExit("The playground was asked to terminate");
        return NSTerminateLater;
    }

    return NSTerminateNow;
}

- (void)runAppThread
{
    const CHAR* CommandLine = *GMacCommandLine;
    GPlaygroundResult = PlaygroundMain(&CommandLine, 1);

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        [NSApp terminate:nil];
    }, NSDefaultRunLoopMode, true);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) Sender
{
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification*) Notification
{
    FMacThreadManager::SetupAppThread(self, @selector(runAppThread));
}

@end

int main(int NumArgs, const CHAR** Args)
{
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GMacCommandLine += " ";

        String CurrentArg(Args[Index]);
        if (CurrentArg.Contains(' '))
        {
            if (CurrentArg.Contains('='))
            {
                String Argument;
                String ArgumentValue;
                CurrentArg.Split('=', Argument, ArgumentValue);

                CurrentArg = String::Printf("%s=\"%s\"", *Argument, *ArgumentValue);
            }
            else
            {
                CurrentArg = String::Printf("\"%s\"", *CurrentArg);
            }
        }

        GMacCommandLine += CurrentArg;
    }

    [NSApplication sharedApplication];
    [NSApp setDelegate:[FPlaygroundAppDelegate new]];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp run];

    FMacThreadManager::ShutdownAppThread();

    return GPlaygroundResult;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
