#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Misc/Config.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Containers/String.h"
#include <Appkit/Appkit.h>
#include <pthread.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

extern int32 EngineMain(const CHAR_T* Args[], int32 NumArgs);

static TArray<FString> GMacArguments;
static int32           GEngineMainResult = 0;

@interface FCocoaAppDelegate : NSObject<NSApplicationDelegate>

- (void)runAppThread;

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

- (void)runAppThread
{
    TArray<const CHAR_T*> ArgumentPointers;
    for (const FString& Argument : GMacArguments)
    {
        ArgumentPointers.Add(*Argument);
    }

    GEngineMainResult = EngineMain(ArgumentPointers.Data(), ArgumentPointers.Size());
    
    if (GEngineMainResult == 0)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
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
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
    FMacThreadManager::SetupAppThread(self, @selector(runAppThread));
}

@end

static bool IsBoolEnabledAtLaunch(const CHAR_T* Section, const CHAR_T* Name)
{
    FStringView CommandLineValue;
    if (CommandLine::FindOption(Name, CommandLineValue))
    {
        if (CommandLineValue.IsEmpty())
        {
            return true;
        }

        bool bFromCommandLine = false;
        if (TTypeFromString<bool>::FromString(FString(CommandLineValue), bFromCommandLine))
        {
            return bFromCommandLine;
        }
    }

    bool bFromConfig = false;
    return GConfig->GetBool(Section, Name, bFromConfig) ? bFromConfig : false;
}

int main(int NumArgs, const CHAR_T** Args)
{
    CommandLine::Initialize(Args + 1, NumArgs - 1);
    FConfig::Initialize();
    FPlatformMisc::PrepareMetalDebugLayerEnvironment(
        IsBoolEnabledAtLaunch(TEXT("RHI"), TEXT("RHI.EnableDebugLayer")),
        IsBoolEnabledAtLaunch(TEXT("MetalRHI"), TEXT("MetalRHI.EnableShaderValidation")));

    // The first argument is always the path to the application, so start processing from index 1
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GMacArguments.Emplace(Args[Index]);
    }
    
    [NSApplication sharedApplication];
    [NSApp setDelegate:[FCocoaAppDelegate new]];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp run];
    
    FMacThreadManager::ShutdownAppThread();
    
    return GEngineMainResult;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
