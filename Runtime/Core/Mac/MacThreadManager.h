#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

#define APP_THREAD_ENABLED (1)

class FRunLoopSourceContext;

@interface NSThread (FAppThread)

+ (NSThread*) appThread;
+ (BOOL) isAppThread;
- (BOOL) isAppThread;

@end

@interface FAppThread : NSThread

- (id)init;
- (id)initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument;
- (void)main;
- (void)dealloc;

@end

@interface FRunLoopSource : NSObject
{
@private
    FRunLoopSourceContext* Context;
    CFRunLoopRef           ScheduledRunLoop;
    CFStringRef            ScheduledMode;
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
        : RunLoopModes([InRunLoopModes retain]) // Retain the NSArray to maintain ownership
        , Block(Block_copy(InBlock))            // Copy the block to ensure it stays valid
    {
    }

    ~FRunLoopTask()
    {
        Block_release(Block);   // Release the copied block
        [RunLoopModes release]; // Release the retained NSArray
    }

    NSArray*         RunLoopModes;
    dispatch_block_t Block;
};

class FRunLoopSourceContext : public FRefCounted
{
public:
    FRunLoopSourceContext(CFRunLoopRef InRunLoop);
    ~FRunLoopSourceContext();

    void RegisterForMode(CFStringRef InRunLoopMode);
    void ScheduleBlock(dispatch_block_t Block, NSArray* InModes);
    void Execute(CFStringRef InRunLoopMode);
    void WakeUp();

private:

    /** @brief Helper callback used with CFDictionaryApplyFunction to remove and cleanup sources when destructing this context. */
    static void Destroy(const void* Key, const void* Value, void* Context);

    /** @brief Helper callback used with CFDictionaryApplyFunction to signal all sources, prompting them to perform tasks if ready. */
    static void Signal(const void* Key, const void* Value, void* Context);

    /** @brief CFRunLoop callback that is invoked when a source is scheduled in a particular mode. */
    static void Schedule(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode);

    /** @brief CFRunLoop callback that is invoked when a source is canceled from a particular mode. */
    static void Cancel(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode);

    /** @brief CFRunLoop callback that performs the run loop source’s associated work. */
    static void Perform(void* Info);

private:
    CFRunLoopRef           RunLoop;
    CFMutableDictionaryRef SourceAndModeDictionary;
    FCriticalSection       SourceAndModeCS;
    TQueue<FRunLoopTask*>  Tasks; 
    FCriticalSection       TasksCS;
};

class CORE_API FMacThreadManager
{
public:
    static bool SetupAppThread(id Delegate, SEL AppThreadEntry);
    static void ShutdownAppThread();

    static void PumpMessagesAppThread(bool bUntilEmpty);

    static FORCEINLINE FMacThreadManager& Get()
    {
        return GMacThreadManager;
    }

public:
    void RegisterMainThreadRunLoop();
    void RegisterAppThreadRunLoop();

    void MainThreadDispatch(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);
    void AppThreadDispatch(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);

    template<typename ReturnType>
    inline ReturnType MainThreadDispatchAndReturn(ReturnType (^Block)(void), NSString* WaitMode)
    {
        __block ReturnType ReturnValue;
        MainThreadDispatch(^
        {
            ReturnValue = Block();
        }, WaitMode, true);
        
        return ReturnValue;
    }

    template<typename ReturnType>
    inline ReturnType AppThreadDispatchAndReturn(ReturnType (^Block)(void), NSString* WaitMode)
    {
        __block ReturnType ReturnValue;
        AppThreadDispatch(^
        {
            ReturnValue = Block();
        }, WaitMode, true);

        return ReturnValue;
    }

private:
    FMacThreadManager();
    ~FMacThreadManager();

    void DispatchOnThread(FRunLoopSourceContext* SourceContext, bool bAlreadyOnTargetThread, dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished);
    void DestroyContexts();

    FRunLoopSourceContext* MainThreadContext;
    FRunLoopSourceContext* AppThreadContext;

    static FMacThreadManager GMacThreadManager;
};
