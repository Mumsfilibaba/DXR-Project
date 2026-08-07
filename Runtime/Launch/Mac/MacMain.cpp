#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Containers/String.h"
#include <Appkit/Appkit.h>
#include <pthread.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

// Forward declaration of the EngineMain function, which serves as the entry point for the engine
extern int32 EngineMain(const CHAR* Args[], int32 NumArgs);

static String GMacCommandLine;     // Stores the command-line arguments as a single string
static int32 GEngineMainResult = 0; // Stores the result returned by EngineMain

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
    
    // If EngineMain indicates success or a specific condition, schedule application termination
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

int main(int NumArgs, const CHAR** Args)
{
    // The first argument is always the path to the application, so start processing from index 1
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GMacCommandLine += " ";
        String CurrentArg(Args[Index]);
        
        // If the current argument contains spaces, handle it appropriately
        if (CurrentArg.Contains(' '))
        {
            if (CurrentArg.Contains('='))
            {
                String Argument;
                String ArgumentValue;
                CurrentArg.Split('=', Argument, ArgumentValue);
                
                // Format as key="value" to handle spaces within the value
                CurrentArg = String::CreateFormatted("%s=\"%s\"", *Argument, *ArgumentValue);
            }
            else
            {
                // Wrap the entire argument in quotes to handle spaces
                CurrentArg = String::CreateFormatted("\"%s\"", *CurrentArg);
            }
        }
        
        GMacCommandLine += CurrentArg; // Append the formatted argument to the command line string
    }
    
    [NSApplication sharedApplication];                                // Get the shared NSApplication instance
    [NSApp setDelegate:[FCocoaAppDelegate new]];                      // Set the application delegate to an instance of FCocoaAppDelegate
    [NSApp activateIgnoringOtherApps:YES];                            // Activate the application, bringing it to the foreground
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];  // Set presentation options to default
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular]; // Set activation policy to regular (shows dock icon and menu bar)
    [NSApp run];                                                      // Start the application's main run loop
    
    // After the run loop exits, shut down the application thread
    FMacThreadManager::ShutdownAppThread();
    
    return GEngineMainResult; // Return the result from EngineMain as the exit code
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
