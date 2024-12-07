#include "Core/Mac/MacRunLoop.h"
#include "Core/Mac/ScopedAutoreleasePool.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Atomic.h"
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

#define APP_THREAD_STACK_SIZE (128 * 1024 * 1024)

static NSThread* GAppThread = nil;

/**
 * @brief Extends NSThread to introduce the concept of an "application thread" as a counterpart to the main thread.
 *
 * This category leverages a global reference (GAppThread) to a specific NSThread instance, treating it
 * as the designated application thread. Through these methods, it becomes easy to check whether the current 
 * thread is the appointed application thread or to retrieve a handle to it. If no application thread has been 
 * set, these methods transparently fall back to the main thread, ensuring a consistent and stable default.
 *
 * - `+(NSThread*) appThread` attempts to return the globally recognized application thread. If none has been 
 *   established, it returns the main thread. This provides a unified way to retrieve the special application 
 *   thread for code paths that need to be sure they are running on the correct thread context.
 *
 * - `+(BOOL) isAppThread` and `-(BOOL) isAppThread` determine if the current or a given thread instance is the 
 *   application thread, respectively. This can be critical in scenarios where you must guarantee certain tasks 
 *   run on this thread, similarly to how one ensures tasks run on the main thread in a UI-centric application.
 *
 * Together, these extensions allow developers to define, identify, and rely upon a particular thread as the 
 * designated application thread, enhancing control over threading models and execution policies within the 
 * application.
 */
@implementation NSThread (FAppThread)

+(NSThread*) appThread
{
    if (!GAppThread)
    {
        return [NSThread mainThread];
    }

    return GAppThread;
}

+(BOOL) isAppThread
{
    const bool bIsAppThread = [[NSThread currentThread] isAppThread];
    return bIsAppThread;
}

-(BOOL) isAppThread
{
    const bool bIsAppThread = self == GAppThread;
    return bIsAppThread;
}

@end

/**
 * @brief Implements the FAppThread class, a dedicated NSThread subclass for the application’s primary thread.
 *
 * This implementation ensures that a global reference (GAppThread) is maintained to identify this 
 * thread as the "application thread." When initialized, the thread’s global pointer is set, allowing 
 * the rest of the application to reliably reference it. This is comparable to the main thread concept, 
 * but for a custom-defined "AppThread."
 *
 * Key behaviors:
 * - During initialization (`-init` and `-initWithTarget:selector:object:`), this thread instance 
 *   assigns itself to GAppThread, establishing it as the official application thread.
 * - In the `-main` method, the thread’s scheduling parameters are adjusted to give it a high 
 *   priority. It then registers a run loop specific to this thread, sets a human-readable name 
 *   ("AppThread"), and invokes its superclass’s `-main` to run any queued tasks.
 *   After the run loop finishes, it coordinates with the main thread to restore conditions for 
 *   sudden termination, depending on whether the engine is requested to exit.
 * - Upon deallocation (`-dealloc`), it clears the global reference to this thread (GAppThread), 
 *   ensuring other parts of the code no longer mistakenly reference a destroyed thread.
 *
 * By providing a well-defined, globally identifiable "AppThread," this implementation facilitates 
 * thread-specific operations and event handling that may be critical in complex, multi-threaded 
 * applications.
 */
@implementation FAppThread

-(id) init
{
    self = [super init];
    if (self)
    {
        GAppThread = self;
    }
    
    return self;
}

-(id) initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument
{
    self = [super initWithTarget:Target selector:Selector object:Argument];
    if (self)
    {
        GAppThread = self;
    }
    
    return self;
}

-(void) main
{
    struct sched_param SchedParams;
    FMemory::Memzero(&SchedParams, sizeof(SchedParams));
    
    int32 Policy = SCHED_RR;
    pthread_getschedparam(pthread_self(), &Policy, &SchedParams);

    SchedParams.sched_priority = sched_get_priority_max(2);
    pthread_setschedparam(pthread_self(), Policy, &SchedParams);
    
    // Register the runloop for this current thread
    FRunLoopSourceContext::RegisterAppThreadRunLoop();
    
    // Set our name
    [self setName:@"AppThread"];
    
    // Run the thread
    [super main];
    
    // Restore the sudden termination state
    if (IsEngineExitRequested())
    {
        FMacThreadManager::ExecuteOnMainThread(^{
            [NSApp replyToApplicationShouldTerminate:YES];
            [[NSProcessInfo processInfo] enableSuddenTermination];
        }, NSDefaultRunLoopMode, false);
    }
    else
    {
        FMacThreadManager::ExecuteOnMainThread(^{
            [[NSProcessInfo processInfo] enableSuddenTermination];
        }, NSDefaultRunLoopMode, false);
    }
}

-(void) dealloc
{
    GAppThread = nullptr;
    [super dealloc];
}

@end

/**
 * @brief Implements the FRunLoopSource class, a thin Objective-C wrapper around a CFRunLoopSourceContext-based source.
 *
 * This implementation manages the lifecycle of a run loop source and provides methods 
 * to schedule, cancel, and perform actions associated with that source on a specified 
 * run loop and mode. By encapsulating the raw CFRunLoopSourceContext pointer, it ensures 
 * that the underlying context is properly retained and released, and that the source 
 * is consistently managed.
 *
 * FRunLoopSource bridges the gap between low-level Core Foundation run loop sources and 
 * higher-level Objective-C code. It initializes with a given context, increasing its 
 * reference count for lifetime management. Once scheduled on a run loop with a particular mode, 
 * it can later be canceled or triggered to perform its defined action.
 *
 * The perform method confirms that it is being executed in the correct context (i.e., 
 * the correct run loop and mode), ensuring that the source’s callbacks fire only under 
 * the intended conditions. This pattern helps avoid concurrency issues, ensures that 
 * cleanup occurs as expected, and simplifies interaction with run loop sources.
 */
@implementation FRunLoopSource

- (id)initWithContext:(FRunLoopSourceContext*)InContext
{
    id Self = [super init];
    if(Self)
    {
        CHECK(InContext);
        Context = InContext;
        Context->AddRef(); // Increase ref count to manage lifetime of the context
    }

    return Self;
}

- (void)dealloc
{
    CHECK(Context);
    Context->Release(); // Release the retained context before dealloc
    Context = nullptr;
    [super dealloc];
}

- (void)scheduleOn:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode
{
    CHECK(!ScheduledRunLoop);
    CHECK(!ScheduledMode);
    ScheduledRunLoop = InRunLoop; // Remember which run loop we’re scheduled on
    ScheduledMode    = InMode;    // And in which mode
}

- (void)cancelFrom:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode
{
    // If currently scheduled in the same run loop and mode, clear the scheduling info.
    if(CFEqual(InRunLoop, ScheduledRunLoop) && CFEqual(ScheduledMode, InMode))
    {
        ScheduledRunLoop = nullptr;
        ScheduledMode    = nullptr;
    }
}

- (void)perform
{
    CHECK(Context);
    CHECK(ScheduledRunLoop);
    CHECK(ScheduledMode);
    CHECK(CFEqual(ScheduledRunLoop, CFRunLoopGetCurrent()));
    
    CFStringRef CurrentMode = CFRunLoopCopyCurrentMode(CFRunLoopGetCurrent());
    CHECK(CFEqual(CurrentMode, ScheduledMode));
    
    // Execute the context’s callback in the current mode.
    Context->Execute(CurrentMode);
    CFRelease(CurrentMode);
}

@end

FRunLoopSourceContext* FRunLoopSourceContext::MainThreadContext        = nullptr;
FRunLoopSourceContext* FRunLoopSourceContext::AppThreadContext = nullptr;

FRunLoopSourceContext& FRunLoopSourceContext::GetMainThreadContext()
{
    CHECK(MainThreadContext != nullptr);
    return *MainThreadContext;
}

FRunLoopSourceContext& FRunLoopSourceContext::GetAppThreadContext()
{
    CHECK(AppThreadContext != nullptr);
    return *AppThreadContext;
}

bool FRunLoopSourceContext::RegisterMainThreadRunLoop()
{
    CHECK(FPlatformThreadMisc::IsMainThread());
    CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
    MainThreadContext = new FRunLoopSourceContext(RunLoop);
    return true;
}

bool FRunLoopSourceContext::RegisterAppThreadRunLoop()
{
    CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
    AppThreadContext = new FRunLoopSourceContext(RunLoop);
    return true;
}

FRunLoopSourceContext::FRunLoopSourceContext(CFRunLoopRef InRunLoop)
    : RunLoop(InRunLoop)
    , SourceAndModeDictionary(CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks))
    , Tasks()
{
    CFRetain(RunLoop);

    // Register common modes
    RegisterForMode(kCFRunLoopDefaultMode);
    RegisterForMode((CFStringRef)NSModalPanelRunLoopMode);
}

FRunLoopSourceContext::~FRunLoopSourceContext()
{
    // Clear global pointers if this instance is one of them
    if (this == MainThreadContext)
    {
        MainThreadContext = nullptr;
    }
    
    if (this == AppThreadContext)
    {
        AppThreadContext = nullptr;
    }

    // Remove all sources associated with this run loop
    CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Destroy, RunLoop);

    CFRelease(SourceAndModeDictionary);
    CFRelease(RunLoop);
}

void FRunLoopSourceContext::RegisterForMode(CFStringRef InRunLoopMode)
{
    // Only register if not already present
    if(!CFDictionaryContainsKey(SourceAndModeDictionary, InRunLoopMode))
    {
        // Create a new run loop source for this mode
        FRunLoopSource* RunLoopSource = [[FRunLoopSource alloc] initWithContext:this];

        CFRunLoopSourceContext SourceContext = CFRunLoopSourceContext();
        SourceContext.info    = reinterpret_cast<void*>(RunLoopSource);
        SourceContext.version = 0;

        // Set custom CFRunLoop callbacks
        SourceContext.retain          = CFRetain;
        SourceContext.release         = CFRelease;
        SourceContext.copyDescription = CFCopyDescription;
        SourceContext.equal           = CFEqual;
        SourceContext.hash            = CFHash;
        SourceContext.perform         = &FRunLoopSourceContext::Perform;
        SourceContext.schedule        = &FRunLoopSourceContext::Schedule;
        SourceContext.cancel          = &FRunLoopSourceContext::Cancel;

        // Create and add the CFRunLoopSource to the dictionary and run loop
        CFRunLoopSourceRef Source = CFRunLoopSourceCreate(nullptr, 0, &SourceContext);
        CFDictionaryAddValue(SourceAndModeDictionary, InRunLoopMode, Source);
        CFRunLoopAddSource(RunLoop, Source, InRunLoopMode);

        CFRelease(Source);
    }
}

void FRunLoopSourceContext::ScheduleBlock(dispatch_block_t Block, NSArray* InModes)
{
    // Ensure all requested modes are registered
    for (NSString* Mode in InModes)
    {
        RegisterForMode((CFStringRef)Mode);
    }

    {
        // Add the new task to the queue
        SCOPED_LOCK(TasksCS);
        Tasks.Emplace(new FRunLoopTask(InModes, Block));
    }
    
    // Signal all sources to inform them a new task is ready
    CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Signal, nullptr);
}

void FRunLoopSourceContext::Execute(CFStringRef InRunLoopMode)
{
    // Grab all tasks currently in the queue
    TArray<FRunLoopTask*> NewTasks;
    {
        SCOPED_LOCK(TasksCS);
        Tasks.DequeueAll(NewTasks);
    }

    bool bDone = false;
    while (!bDone)
    {
        bDone = true;

        // Process tasks that apply to the current mode
        for (int32 Index = 0; Index < NewTasks.Size(); Index++)
        {
            FRunLoopTask* Task = NewTasks[Index];
            if (Task && [Task->RunLoopModes containsObject:(NSString*)InRunLoopMode])
            {
                // Execute the task
                NewTasks.RemoveAt(Index);
                Task->Block();
                delete Task;
                
                bDone = false;
                break; // Break and restart the loop to re-check the updated array
            }
        }
    }
}

void FRunLoopSourceContext::RunInMode(CFStringRef RunMode)
{
    // Run the run loop in the given mode until no more events
    CFRunLoopRunInMode(RunMode, 0, true);
}

void FRunLoopSourceContext::WakeUp()
{
    // Wake the run loop to process pending tasks
    CFRunLoopWakeUp(RunLoop);
}

void FRunLoopSourceContext::Destroy(const void* Key, const void* Value, void* Context)
{
    CFRunLoopRef RunLoop = static_cast<CFRunLoopRef>(Context);
    if(RunLoop)
    {
        CFStringRef        RunMode = (CFStringRef)Key;
        CFRunLoopSourceRef Source  = (CFRunLoopSourceRef)Value;
        // Remove the source from the run loop
        CFRunLoopRemoveSource(RunLoop, Source, RunMode);
    }
}

void FRunLoopSourceContext::Signal(const void* Key, const void* Value, void* Context)
{
    CFRunLoopSourceRef RunLoopSource = (CFRunLoopSourceRef)Value;
    if(RunLoopSource)
    {
        // Signal the source that it should perform
        CFRunLoopSourceSignal(RunLoopSource);
    }
}

void FRunLoopSourceContext::Schedule(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode)
{
    FRunLoopSource* RunLoopSource = reinterpret_cast<FRunLoopSource*>(Info);
    if (RunLoopSource)
    {
        [RunLoopSource scheduleOn:InRunLoop inMode:InRunLoopMode];
    }
}

void FRunLoopSourceContext::Cancel(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode)
{
    FRunLoopSource* RunLoopSource = reinterpret_cast<FRunLoopSource*>(Info);
    if (RunLoopSource)
    {
        [RunLoopSource cancelFrom:InRunLoop inMode:InRunLoopMode];
    }
}

void FRunLoopSourceContext::Perform(void* Info)
{
    FRunLoopSource* RunLoopSource = reinterpret_cast<FRunLoopSource*>(Info);
    if (RunLoopSource)
    {
        // Execute the run loop source’s perform logic
        [RunLoopSource perform];
    }
}

bool FMacThreadManager::SetupAppThread(id Delegate, SEL AppThreadEntry)
{
    // Disable sudden termination to prevent the app from quitting unexpectedly.
    [[NSProcessInfo processInfo] disableSuddenTermination];
    
    // Register the main thread's run loop context.
    FRunLoopSourceContext::RegisterMainThreadRunLoop();
    
#if APP_THREAD_ENABLED
    // Initialize and start the AppThread with the provided delegate and selector.
    FAppThread* AppThread = [[FAppThread alloc] initWithTarget:Delegate selector:AppThreadEntry object:nil];
    [AppThread setStackSize:APP_THREAD_STACK_SIZE];
    [AppThread start];
#else
    // If AppThread is not enabled, perform the selector on the delegate directly.
    [Delegate performSelector:AppThreadEntry withObject:nil];
    
    // If an engine exit has been requested, reply to the application termination request.
    if (IsEngineExitRequested())
    {
        [NSApp replyToApplicationShouldTerminate:YES];
    }
#endif
    
    return true;
}

void FMacThreadManager::ShutdownAppThread()
{
    // Release the global AppThread reference.
    [GAppThread release];
    GAppThread = nullptr;
}

void FMacThreadManager::PumpMessagesAppThread(bool bUntilEmpty)
{
    SCOPED_AUTORELEASE_POOL();

#if APP_THREAD_ENABLED
    // Run the run loop in the default mode without a timeout.
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
#else
    // Ensure the NSApp is valid.
    CHECK(NSApp != nil);

    do
    {
        // Retrieve the next event matching any event mask, without waiting (distantPast).
        NSEvent* Event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (!Event)
        {
            break; // Exit if there are no more events.
        }

        // Prevent sending events from invalid windows.
        if ([Event windowNumber] == 0 || [Event window] != nil)
        {
            [NSApp sendEvent:Event];
        }
    } while (bUntilEmpty);
#endif
}

void FMacThreadManager::ExecuteOnThread(FRunLoopSourceContext& SourceContext, dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    // Copy the block to manage its memory correctly.
    dispatch_block_t CopiedBlock = Block_copy(Block);

    if (FPlatformThreadMisc::IsMainThread())
    {
        // If already on the main thread, execute the block immediately.
        CopiedBlock();
    }
    else
    {
        // Otherwise, schedule the block on the specified run loop context.
        SCOPED_AUTORELEASE_POOL();

        // Define the run loop modes in which the block should be executed.
        NSArray* ScheduleModes = @[NSDefaultRunLoopMode, NSModalPanelRunLoopMode, NSEventTrackingRunLoopMode];

        if (bWaitUntilFinished)
        {
            // If waiting for completion, create a semaphore to signal completion.
            __block dispatch_semaphore_t WaitSemaphore = dispatch_semaphore_create(0);

            // Create a waitable block that signals the semaphore after execution.
            dispatch_block_t WaitableBlock = Block_copy(^
            {
                CopiedBlock();
                dispatch_semaphore_signal(WaitSemaphore);
            });

            // Schedule the waitable block on the run loop context.
            SourceContext.ScheduleBlock(WaitableBlock, ScheduleModes);

            // Continuously run the run loop until the semaphore is signaled.
            do
            {
                CFStringRef CurrentMode = (CFStringRef)WaitMode;
                SourceContext.WakeUp();
                SourceContext.RunInMode(CurrentMode);
            } while (dispatch_semaphore_wait(WaitSemaphore, dispatch_time(0, 100000ull)));

            // Release the waitable block and semaphore.
            Block_release(WaitableBlock);
            dispatch_release(WaitSemaphore);
        }
        else
        {
            // If not waiting for completion, simply schedule the block.
            SourceContext.ScheduleBlock(CopiedBlock, ScheduleModes);
            SourceContext.WakeUp();
        }
    }

    // Release the copied block.
    Block_release(CopiedBlock);
}

void FMacThreadManager::ExecuteOnMainThread(dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    // Retrieve the main thread's run loop context.
    FRunLoopSourceContext& SourceContext = FRunLoopSourceContext::GetMainThreadContext();

    // Execute the block on the main thread.
    ExecuteOnThread(SourceContext, Block, WaitMode, bWaitUntilFinished);
}

void FMacThreadManager::ExecuteOnAppThread(dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    // Retrieve the application thread's run loop context.
    FRunLoopSourceContext& SourceContext = FRunLoopSourceContext::GetAppThreadContext();

    // Execute the block on the application thread.
    ExecuteOnThread(SourceContext, Block, WaitMode, bWaitUntilFinished);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
