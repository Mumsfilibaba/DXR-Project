#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Misc/Config.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Containers/String.h"
#include <Appkit/Appkit.h>
#include <pthread.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

extern int32 EngineMain(const CHAR* Args[], int32 NumArgs);

static String GMacCommandLine;
static int32  GEngineMainResult = 0;

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
    const char* CommandLine = *GMacCommandLine;
    GEngineMainResult = EngineMain(&CommandLine, 1);
    
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

// Resolves RHI.EnableDebugLayer the way the console variable would later, with the command line
// outranking the config layers, because the variable itself does not exist yet at this point.
static bool IsDebugLayerEnabledAtLaunch()
{
    StringView CommandLineValue;
    if (CommandLine::FindOption("RHI.EnableDebugLayer", CommandLineValue))
    {
        if (CommandLineValue.IsEmpty())
        {
            return true;
        }

        bool bFromCommandLine = false;
        if (TTypeFromString<bool>::FromString(String(CommandLineValue), bFromCommandLine))
        {
            return bFromCommandLine;
        }
    }

    bool bFromConfig = false;
    return GConfig->GetBool("RHI", "RHI.EnableDebugLayer", bFromConfig) ? bFromConfig : false;
}

int main(int NumArgs, const CHAR** Args)
{
    // AppKit creates the first MTLDevice below, which is where Metal latches its debug layer, so the
    // layer has to be armed here rather than anywhere the console variable would already exist.
    // argv[0] is a path, and a dash anywhere in it would be read as an option.
    CommandLine::Initialize(Args + 1, NumArgs - 1);
    FConfig::Initialize();
    FPlatformMisc::PrepareMetalDebugLayerEnvironment(IsDebugLayerEnabledAtLaunch());

    // The first argument is always the path to the application, so start processing from index 1
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GMacCommandLine += " ";
        String CurrentArg(Args[Index]);
        
        // An argument carrying a space has to reach the command line quoted, and a key=value pair
        // takes the quotes around the value alone so that the key still parses.
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
    [NSApp setDelegate:[FCocoaAppDelegate new]];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];

    // Regular gets the dock icon and the menu bar, which a bare executable does not have by default
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp run];
    
    FMacThreadManager::ShutdownAppThread();
    
    return GEngineMainResult;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
