#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Containers/String.h"
#include "Core/Misc/CommandLine.h"
#include "LaunchProgram/ProgramEntry.h"
#include "LaunchProgram/Mac/MacProgramDelegate.h"
#include <Appkit/Appkit.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

static String GMacCommandLine;
static int32  GProgramMainResult = 0;

@implementation FMacProgramDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*) Sender
{
    if (!FProgramLoop::IsExitRequested())
    {
        FProgramLoop::RequestExit("Application terminate");
        return NSTerminateLater;
    }

    return NSTerminateNow;
}

- (void)runAppThread
{
    const CHAR* CommandLine = *GMacCommandLine;
    CommandLine::Initialize(&CommandLine, 1);
    GProgramMainResult = FProgramLoop::Run(GProgramTitle, GProgramBody);

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        [NSApp terminate:nil];
    }, NSDefaultRunLoopMode, true);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) Sender
{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*) Notification
{
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
    [NSApp setDelegate:[FMacProgramDelegate new]];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp run];

    FMacThreadManager::ShutdownAppThread();
    return GProgramMainResult;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
