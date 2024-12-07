#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Containers/String.h"
#include <Appkit/Appkit.h>
#include <pthread.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

// Forward declaration of the EngineMain function, which serves as the entry point for the engine
extern int32 EngineMain(const CHAR* Args[], int32 NumArgs);

static FString GMacCommandLine;     // Stores the command-line arguments as a single string
static int32 GEngineMainResult = 0; // Stores the result returned by EngineMain

/**
 * @brief Objective-C interface for the Cocoa application delegate. FCocoaAppDelegate conforms to the
 * NSApplicationDelegate protocol and manages application-level events, such as launching, termination,
 * and run loop execution.
 */
@interface FCocoaAppDelegate : NSObject<NSApplicationDelegate>

- (void)runAppThread;

@end

/**
 * @brief Implementation of the FCocoaAppDelegate. This class handles key application events and manages the
 * lifecycle of the application thread's run loop.
 */
@implementation FCocoaAppDelegate

/**
 * @brief Determines whether the application should terminate. This delegate method is called when the application
 * is about to terminate. It checks if an engine exit has been requested. If not, it defers termination; otherwise,
 * it allows immediate termination.
 * @param Sender The NSApplication instance sending the message.
 * @return NSTerminateReply indicating whether to terminate immediately or defer.
 */
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

/**
 * @brief Executes the application thread's main loop. This method is invoked to start the engine's main loop on a
 * dedicated application thread. It calls EngineMain with the provided command-line arguments. If EngineMain returns 0,
 * indicating a successful or specific exit condition, it schedules the application to terminate gracefully.
 */
- (void)runAppThread
{
    const char* CommandLine = *GMacCommandLine;
    GEngineMainResult = EngineMain(&CommandLine, 1);
    
    // If EngineMain indicates success or a specific condition, schedule application termination
    if (GEngineMainResult == 0)
    {
        FMacThreadManager::ExecuteOnMainThread(^
        {
            [NSApp terminate:nil];
        }, NSDefaultRunLoopMode, true);
    }
}

/**
 * @brief Determines whether the application should terminate after the last window is closed. This delegate method is
 * called when the last window is closed. Returning YES allows the application to terminate automatically without
 * requiring the user to quit explicitly.
 * @param Sender The NSApplication instance sending the message.
 * @return YES to terminate after the last window is closed.
 */
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) Sender
{
    return YES;
}

/**
 * @brief Handles additional tasks before the application terminates. This delegate method is called just before the
 * application terminates. Currently, it performs no actions, but it can be overridden to handle cleanup tasks or save
 * state as needed.
 * @param Notification The notification object containing information about the termination.
 */
- (void)applicationWillTerminate:(NSNotification*) Notification
{
}

/**
 * @brief Called when the application has finished launching. This delegate method is invoked after the application has
 * been launched and initialized. It sets up the application thread by calling FMacThreadManager::SetupAppThread with
 * itself and the selector to run the application thread's main loop.
 * @param Notification The notification object containing information about the launch.
 */
- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
    FMacThreadManager::SetupAppThread(self, @selector(runAppThread));
}

@end

/**
 * @brief The main entry point of the application. This function processes command-line arguments, sets up the
 * NSApplication instance and its delegate, activates the application, and starts the main run loop. After the
 * run loop exits, it shuts down the application thread and returns the result from EngineMain.
 * @param NumArgs The number of command-line arguments.
 * @param Args The array of command-line argument strings.
 * @return The exit code returned by EngineMain.
 */
int main(int NumArgs, const CHAR** Args)
{
    // The first argument is always the path to the application, so start processing from index 1
    for (int32 Index = 1; Index < NumArgs; Index++)
    {
        GMacCommandLine += " ";
        FString CurrentArg(Args[Index]);
        
        // If the current argument contains spaces, handle it appropriately
        if (CurrentArg.Contains(' '))
        {
            if (CurrentArg.Contains('='))
            {
                FString Argument;
                FString ArgumentValue;
                CurrentArg.Split('=', Argument, ArgumentValue);
                
                // Format as key="value" to handle spaces within the value
                CurrentArg = FString::CreateFormatted("%s=\"%s\"", *Argument, *ArgumentValue);
            }
            else
            {
                // Wrap the entire argument in quotes to handle spaces
                CurrentArg = FString::CreateFormatted("\"%s\"", *CurrentArg);
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
