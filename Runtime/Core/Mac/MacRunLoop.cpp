#include "MacRunLoop.h"
#include "ScopedAutoreleasePool.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Threading/Spinlock.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/AtomicInt.h"
#include "Core/Platform/PlatformThreadMisc.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#define LOGIC_THREAD_STACK_SIZE (128 * 1024 * 1024)

static NSThread* GApplicationThread = nil;

class FRunLoopSourceContext;

@interface FRunLoopSource : NSObject
{
    // Use a NSObject since we want the source to be CFRunLoopSourceContext for lifetime-handling 

@private
    FRunLoopSourceContext* Context;

    CFRunLoopRef ScheduledRunLoop;
    CFStringRef  ScheduledMode;
}

- (id)initWithContext:(FRunLoopSourceContext*)InContext;

- (void)dealloc;

- (void)scheduleOn:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode;
- (void)cancelFrom:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode;

- (void)perform;
@end


struct FRunLoopTask
{
    FRunLoopTask(NSArray* InRunLoopModes, dispatch_block_t InBlock)
        : RunLoopModes([InRunLoopModes retain])
        , Block(Block_copy(InBlock))
    {
    }
    
    ~FRunLoopTask()
    {
        Block_release(Block);
        [RunLoopModes release];
     }
    
    NSArray*         RunLoopModes;
    dispatch_block_t Block;
};


/** @brief - Manager for a RunLoop for a certain Thread */
class FRunLoopSourceContext : public FRefCounted
{
public:
    static FRunLoopSourceContext& GetMainThreadContext()
    {
        CHECK(MainThreadContext != nullptr);
        return *MainThreadContext;
    }
    
    static FRunLoopSourceContext& GetApplicationThreadContext()
    {
        CHECK(ApplicationThreadContext != nullptr);
        return *ApplicationThreadContext;
    }
    
    static bool RegisterMainThreadRunLoop()
    {
        CHECK(FPlatformThreadMisc::IsMainThread());
        CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
        MainThreadContext = new FRunLoopSourceContext(RunLoop);
        return true;
    }
    
    static bool RegisterApplicationThreadRunLoop()
    {
        CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
        ApplicationThreadContext = new FRunLoopSourceContext(RunLoop);
        return true;
    }
    
    FRunLoopSourceContext(CFRunLoopRef InRunLoop)
        : RunLoop(InRunLoop)
        , SourceAndModeDictionary(CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks))
        , Tasks()
    {
        CFRetain(RunLoop);

        // Register for the default mode
        RegisterForMode(kCFRunLoopDefaultMode);
        RegisterForMode((CFStringRef)NSModalPanelRunLoopMode);
    }
    
    ~FRunLoopSourceContext()
    {
        if (this == MainThreadContext)
        {
            MainThreadContext = nullptr;
        }
        
        if (this == ApplicationThreadContext)
        {
            ApplicationThreadContext = nullptr;
        }
        
        // This dictionary is created so that keys and values are automatically retained and released when added or removed
        CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Destroy, RunLoop);
        
        CFRelease(SourceAndModeDictionary);
        CFRelease(RunLoop);
    }
    
    void RegisterForMode(CFStringRef InRunLoopMode)
    {
        // Register for a new mode if the source is not already registered
        if(CFDictionaryContainsKey(SourceAndModeDictionary, InRunLoopMode) == false)
        {
            FRunLoopSource* RunLoopSource = [[FRunLoopSource alloc] initWithContext:this];

            CFRunLoopSourceContext SourceContext = CFRunLoopSourceContext();
            SourceContext.info    = reinterpret_cast<void*>(RunLoopSource);
            SourceContext.version = 0;

            // These functions operate on the Info pointer
            SourceContext.retain          = CFRetain;
            SourceContext.release         = CFRelease;
            SourceContext.copyDescription = CFCopyDescription;
            SourceContext.equal           = CFEqual;
            SourceContext.hash            = CFHash;
            
            // Setup so that the context is called via the FRunLoopSource
            SourceContext.perform  = &FRunLoopSourceContext::Perform;
            SourceContext.schedule = &FRunLoopSourceContext::Schedule;
            SourceContext.cancel   = &FRunLoopSourceContext::Cancel;

            CFRunLoopSourceRef Source = CFRunLoopSourceCreate(nullptr, 0, &SourceContext);

            // This dictionary is created so that keys and values are automatically retained and released when added or removed
            CFDictionaryAddValue(SourceAndModeDictionary, InRunLoopMode, Source);
            CFRunLoopAddSource(RunLoop, Source, InRunLoopMode);

            CFRelease(Source);
        }
    }

    void ScheduleBlock(dispatch_block_t Block, NSArray* InModes)
    {
        for (NSString* Mode in InModes)
        {
            RegisterForMode((CFStringRef)Mode);
        }

        // Enqueue a new task
        Tasks.Emplace(new FRunLoopTask(InModes, Block));
        
        // Signal the source
        CFDictionaryApplyFunction(SourceAndModeDictionary, &FRunLoopSourceContext::Signal, nullptr);
    }
    
    void Execute(CFStringRef InRunLoopMode)
    {
        // Take all the tasks that currently exists
        TArray<FRunLoopTask*> NewTasks;
        Tasks.DequeueAll(NewTasks);
        
        bool bDone = false;
        while (!bDone)
        {
            // Ensure we exit if we do not process any tasks
            bDone = true;
            
            // Execute the task
            for (int32 Index = 0; Index < NewTasks.Size(); Index++)
            {
                FRunLoopTask* Task = NewTasks[Index];
                if (Task && [Task->RunLoopModes containsObject:(NSString*)InRunLoopMode])
                {
                    NewTasks.RemoveAt(Index);
                    Task->Block();
                    delete Task;
                    bDone = false;
                    break;
                }
            }
        }
    }
    
    void RunInMode(CFStringRef RunMode)
    {
        CFRunLoopRunInMode(RunMode, 0, true);
    }
    
    void WakeUp()
    {
        CFRunLoopWakeUp(RunLoop);
    }
    
private:
    static void Destroy(const void* Key, const void* Value, void* Context)
    {
        CFRunLoopRef RunLoop = static_cast<CFRunLoopRef>(Context);
        if(RunLoop)
        {
            CFStringRef        RunMode = (CFStringRef)Key;
            CFRunLoopSourceRef Source  = (CFRunLoopSourceRef)Value;
            CFRunLoopRemoveSource(RunLoop, Source, RunMode);
        }
    }

    static void Signal(const void* Key, const void* Value, void* Context)
    {
        CFRunLoopSourceRef RunLoopSource = (CFRunLoopSourceRef)Value;
        if(RunLoopSource)
        {
            CFRunLoopSourceSignal(RunLoopSource);
        }
    }

    static void Schedule(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode)
    {
        FRunLoopSource* RunLoopSource = reinterpret_cast<FRunLoopSource*>(Info);
        if (RunLoopSource)
        {
            [RunLoopSource scheduleOn:InRunLoop inMode:InRunLoopMode];
        }
    }

    static void Cancel(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode)
    {
        FRunLoopSource* RunLoopSource = reinterpret_cast<FRunLoopSource*>(Info);
        if (RunLoopSource)
        {
            [RunLoopSource cancelFrom:InRunLoop inMode:InRunLoopMode];
        }
    }

    static void Perform(void* Info)
    {
        FRunLoopSource* RunLoopSource = reinterpret_cast<FRunLoopSource*>(Info);
        if (RunLoopSource)
        {
            [RunLoopSource perform];
        }
    }
    
private:
    CFRunLoopRef           RunLoop;
    CFMutableDictionaryRef SourceAndModeDictionary;
    TQueue<FRunLoopTask*, EQueueType::MPSC> Tasks;
    
    static FRunLoopSourceContext* MainThreadContext;
    static FRunLoopSourceContext* ApplicationThreadContext;
};

FRunLoopSourceContext* FRunLoopSourceContext::MainThreadContext;
FRunLoopSourceContext* FRunLoopSourceContext::ApplicationThreadContext;


/** @brief - Interface for the RunLoopSource */
@implementation FRunLoopSource

- (id)initWithContext:(FRunLoopSourceContext*)InContext
{
    id Self = [super init];
    if(Self)
    {
        CHECK(InContext);
        Context = InContext;
        Context->AddRef();
    }

    return Self;
}

- (void)dealloc
{
    CHECK(Context);
    Context->Release();
    Context = nullptr;
    [super dealloc];
}

- (void)scheduleOn:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode
{
    CHECK(!ScheduledRunLoop);
    CHECK(!ScheduledMode);
    ScheduledRunLoop = InRunLoop;
    ScheduledMode    = InMode;
}

- (void)cancelFrom:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode
{
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
    
    Context->Execute(CurrentMode);
    CFRelease(CurrentMode);
}

@end


/** @brief - Extend NSThread in order to check for the ApplicationThread in a similar way as the MainThread */
@implementation NSThread (FApplicationThread)

+(NSThread*) applicationThread
{
    if (!GApplicationThread)
    {
        return [NSThread mainThread];
    }
    
    return GApplicationThread;
}

+(BOOL) isApplicationThread
{
    const BOOL bIsAppThread = [[NSThread currentThread] isApplicationThread];
    return bIsAppThread;
}

-(BOOL) isApplicationThread
{
    const BOOL bIsAppThread = self == GApplicationThread;
    return bIsAppThread;
}

@end


/** @brief - Create a subclass for the ApplicationThread */
@implementation FApplicationThread

-(id) init
{
    self = [super init];
    if (self)
    {
        GApplicationThread = self;
    }
    
    return self;
}

-(id) initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument
{
    self = [super initWithTarget:Target selector:Selector object:Argument];
    if (self)
    {
        GApplicationThread = self;
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
    FRunLoopSourceContext::RegisterApplicationThreadRunLoop();
    
    // Set our name
    [self setName:@"ApplicationThread"];
    
    // Run the thread
    [super main];
    
    // Restore the sudden termination state
    if (IsEngineExitRequested())
    {
        ExecuteOnMainThread(^{
            [NSApp replyToApplicationShouldTerminate:YES];
            [[NSProcessInfo processInfo] enableSuddenTermination];
        }, NSDefaultRunLoopMode, false);
    }
    else
    {
        ExecuteOnMainThread(^{
            [[NSProcessInfo processInfo] enableSuddenTermination];
        }, NSDefaultRunLoopMode, false);
    }
}

-(void) dealloc
{
    GApplicationThread = nullptr;
    [super dealloc];
}

@end


/** @brief - Interface for starting up the ApplicationThread */
bool SetupApplicationThread(id Delegate, SEL ApplicationThreadEntry)
{
    [[NSProcessInfo processInfo] disableSuddenTermination];
    
    FRunLoopSourceContext::RegisterMainThreadRunLoop();

#if APPLICATION_THREAD_ENABLED
    FApplicationThread* ApplicationThread = [[FApplicationThread alloc] initWithTarget:Delegate selector:ApplicationThreadEntry object:nil];
    [ApplicationThread setStackSize:LOGIC_THREAD_STACK_SIZE];
    [ApplicationThread start];
#else
    [Delegate performSelector:ApplicationThreadEntry withObject:nil];
    if (IsEngineExitRequested())
    {
        [NSApp replyToApplicationShouldTerminate:YES];
    }
#endif
    return true;
}


/** @brief - Interface for closing down the ApplicationThread */
void ShutdownApplicationThread()
{
    [GApplicationThread release];
}


/** @brief - Interface for running the current thread's runloop */
void PumpMessagesApplicationThread(bool bUntilEmpty)
{
    SCOPED_AUTORELEASE_POOL();
    
#if APPLICATION_THREAD_ENABLED
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
#else
    CHECK(NSApp != nil);
    
    do
    {
        NSEvent* Event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (!Event)
        {
            break;
        }
        
        // Prevent to send event from invalid windows
        if ([Event windowNumber] == 0 || [Event window] != nil)
        {
            [NSApp sendEvent:Event];
        }
    } while (bUntilEmpty);
#endif
}


/** @brief - Interface for executing a block on either Main- or ApplicationThread */
void ExecuteOnThread(FRunLoopSourceContext& SourceContext, dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    dispatch_block_t CopiedBlock = Block_copy(Block);
    
    if (FPlatformThreadMisc::IsMainThread())
    {
        // If already on mainthread, execute Block here
        CopiedBlock();
    }
    else
    {
        // Otherwise schedule Block on main thread
        SCOPED_AUTORELEASE_POOL();
        
        NSArray* ScheduleModes = @[NSDefaultRunLoopMode, NSModalPanelRunLoopMode, NSEventTrackingRunLoopMode];
        if (bWaitUntilFinished)
        {
            __block dispatch_semaphore_t WaitSemaphore = dispatch_semaphore_create(0);
            dispatch_block_t WaitableBlock = Block_copy(^
            {
                CopiedBlock();
                dispatch_semaphore_signal(WaitSemaphore);
            });
            
            SourceContext.ScheduleBlock(WaitableBlock, ScheduleModes);
            
            do
            {
                CFStringRef CurrentMode = (CFStringRef)WaitMode;
                SourceContext.WakeUp();
                SourceContext.RunInMode(CurrentMode);
            } while (dispatch_semaphore_wait(WaitSemaphore, dispatch_time(0, 100000ull)));
            
            Block_release(WaitableBlock);
            dispatch_release(WaitSemaphore);
        }
        else
        {
            SourceContext.ScheduleBlock(CopiedBlock, ScheduleModes);
            SourceContext.WakeUp();
        }
    }
    
    Block_release(CopiedBlock);
}

void ExecuteOnMainThread(dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    FRunLoopSourceContext& SourceContext = FRunLoopSourceContext::GetMainThreadContext();
    ExecuteOnThread(SourceContext, Block, WaitMode, bWaitUntilFinished);
}

void ExecuteOnAppThread(dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished)
{
    FRunLoopSourceContext& SourceContext = FRunLoopSourceContext::GetApplicationThreadContext();
    ExecuteOnThread(SourceContext, Block, WaitMode, bWaitUntilFinished);
}

#pragma clang diagnostic pop
