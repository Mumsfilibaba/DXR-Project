#include "Core/Mac/MacThreadManager.h"
#include "Core/Mac/ScopedAutoreleasePool.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Atomic.h"
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <pthread/qos.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

#define APP_THREAD_STACK_SIZE (128 * 1024 * 1024)

static NSThread* GAppThread = nil;

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
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);

    // Register the runloop for this current thread
    FMacThreadManager::Get().RegisterAppThreadRunLoop();

    // Set our name
    [self setName:@"AppThread"];

    // Run the thread
    [super main];

    // Restore the sudden termination state
    if (IsEngineExitRequested())
    {
        FMacThreadManager::Get().MainThreadDispatch(^{
            [NSApp replyToApplicationShouldTerminate:YES];
            [[NSProcessInfo processInfo] enableSuddenTermination];
        }, NSDefaultRunLoopMode, false);
    }
    else
    {
        FMacThreadManager::Get().MainThreadDispatch(^{
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
    {
        // Remove all sources associated with this run loop
        SCOPED_LOCK(SourceAndModeCS);
        CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Destroy, RunLoop);
    }

    CFRelease(SourceAndModeDictionary);
    CFRelease(RunLoop);
}

void FRunLoopSourceContext::RegisterForMode(CFStringRef InRunLoopMode)
{
    SCOPED_LOCK(SourceAndModeCS);

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
    SCOPED_LOCK(SourceAndModeCS);
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

    TArray<FRunLoopTask*> DeferredTasks;
    for (FRunLoopTask* Task : NewTasks)
    {
        if (!Task)
        {
            continue;
        }

        if ([Task->RunLoopModes containsObject:(NSString*)InRunLoopMode])
        {
            Task->Block();
            delete Task;
        }
        else
        {
            DeferredTasks.Emplace(Task);
        }
    }

    if (!DeferredTasks.IsEmpty())
    {
        SCOPED_LOCK(TasksCS);
        for (FRunLoopTask* Task : DeferredTasks)
        {
            Tasks.Emplace(Task);
        }
    }
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

FMacThreadManager FMacThreadManager::MacThreadManager;

FMacThreadManager::FMacThreadManager()
    : MainThreadContext(nullptr)
    , AppThreadContext(nullptr)
{
}

FMacThreadManager::~FMacThreadManager()
{
}

void FMacThreadManager::RegisterMainThreadRunLoop()
{
    CHECK_COCOA_MAIN_THREAD();
    CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
    MainThreadContext = new FRunLoopSourceContext(RunLoop);
}

void FMacThreadManager::RegisterAppThreadRunLoop()
{
    CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
    AppThreadContext = new FRunLoopSourceContext(RunLoop);
}

bool FMacThreadManager::SetupAppThread(id Delegate, SEL AppThreadEntry)
{
    // Disable sudden termination to prevent the app from quitting unexpectedly.
    [[NSProcessInfo processInfo] disableSuddenTermination];

    // Register the main thread's run loop context.
    MacThreadManager.RegisterMainThreadRunLoop();

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

    // Destroy the runloop contexts
    MacThreadManager.DestroyContexts();
}

void FMacThreadManager::DestroyContexts()
{
    delete MainThreadContext;
    MainThreadContext = nullptr;

    delete AppThreadContext;
    AppThreadContext = nullptr;
}

void FMacThreadManager::PumpMessagesAppThread(bool bUntilEmpty)
{
    SCOPED_AUTORELEASE_POOL();

#if APP_THREAD_ENABLED
    // Run the run loop in the default mode without a timeout.
    while ((CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false) == kCFRunLoopRunHandledSource) && bUntilEmpty)
    {
    }
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

void FMacThreadManager::DispatchOnThread(FRunLoopSourceContext* SourceContext, bool bAlreadyOnTargetThread, dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    // Copy the block to manage its memory correctly.
    dispatch_block_t CopiedBlock = Block_copy(Block);

    if (bAlreadyOnTargetThread)
    {
        // If already on the target thread, execute the block immediately.
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
            SourceContext->ScheduleBlock(WaitableBlock, ScheduleModes);

            do
            {
                SourceContext->WakeUp();
                CFRunLoopRunInMode((CFStringRef)WaitMode, 0, true);
            } while (dispatch_semaphore_wait(WaitSemaphore, dispatch_time(DISPATCH_TIME_NOW, 100000ull)));

            // Release the waitable block and semaphore.
            Block_release(WaitableBlock);
            dispatch_release(WaitSemaphore);
        }
        else
        {
            // If not waiting for completion, simply schedule the block.
            SourceContext->ScheduleBlock(CopiedBlock, ScheduleModes);
            SourceContext->WakeUp();
        }
    }

    // Release the copied block.
    Block_release(CopiedBlock);
}

void FMacThreadManager::MainThreadDispatch(dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    // Execute the block on the main thread.
    DispatchOnThread(MainThreadContext, FPlatformThreadMisc::IsMainThread(), Block, WaitMode, bWaitUntilFinished);
}

void FMacThreadManager::AppThreadDispatch(dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    // Execute the block on the application thread.
    DispatchOnThread(AppThreadContext, [NSThread isAppThread], Block, WaitMode, bWaitUntilFinished);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
