#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Containers/String.h"
#include "Core/Misc/CommandLine.h"
#include "LaunchProgram/ProgramEntry.h"
#include "LaunchProgram/Mac/MacProgramDelegate.h"
#include <Appkit/Appkit.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

static int32 GProgramMainResult = 0;

@implementation FMacProgramDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*) Sender
{
    FProgramLoop::RequestExit(TEXT("Application terminate"));
    return NSTerminateLater;
}

- (void)runAppThread
{
    TArray<const CHAR_T*> ArgumentPointers;
    for (const FString& Argument : GProgramArgs)
    {
        ArgumentPointers.Add(*Argument);
    }

    CommandLine::Initialize(ArgumentPointers.Data(), ArgumentPointers.Size());
    GProgramMainResult = FProgramLoop::Run(GProgramTitle, GProgramBody);

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        exit(GProgramMainResult);
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

int main(int NumArgs, const CHAR_T** Args)
{
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GProgramArgs.Emplace(Args[Index]);
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
