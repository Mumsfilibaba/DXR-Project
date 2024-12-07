#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

#define APP_THREAD_ENABLED (1)

class FRunLoopSourceContext;

/**
 * @brief Extends NSThread to introduce the concept of an "AppThread," similar to the built-in main thread.
 *
 * This category adds a notion of a dedicated "application thread," allowing you to identify and
 * differentiate this special thread from all others at runtime. It provides methods to access
 * the designated application thread and to determine whether the current thread or a given thread 
 * is the appointed application thread, much like NSThread’s built-in main thread convenience methods.
 *
 * Use `+[NSThread appThread]` to retrieve a reference to the designated application thread once
 * it has been established. The `+[NSThread isAppThread]` and `- [NSThread isAppThread]` methods 
 * allow you to quickly check if the current or a specific NSThread instance is the "AppThread."
 * This can be particularly useful when certain operations or callbacks must only be performed 
 * on a specific, well-defined thread.
 */
@interface NSThread (FAppThread)

/**
 * @brief Returns the reference to the designated application thread.
 * @return The NSThread instance representing the appointed application thread.
 * 
 * Once set, this thread serves as the "AppThread" for your application, similar in concept 
 * to the main thread for UIKit or AppKit.
 */
+ (NSThread*) appThread;

/**
 * @brief Checks if the current thread is the designated application thread.
 * @return YES if the current thread is the AppThread, NO otherwise.
 */
+ (BOOL) isAppThread;

/**
 * @brief Checks if the receiver is the designated application thread.
 * @return YES if the receiver is the AppThread, NO otherwise.
 */
- (BOOL) isAppThread;

@end

/**
 * @brief A specialized NSThread subclass designated as the "AppThread."
 *
 * This class is intended to represent the principal application thread, analogous to the main thread.
 * By subclassing NSThread, you can establish a known "AppThread" environment that may be used 
 * elsewhere in your codebase to ensure certain tasks are always executed on this specific thread.
 *
 * Creating an instance of FAppThread and starting it will spawn a dedicated thread that can be 
 * referenced as the application’s primary thread (beyond the main thread). It can be useful in 
 * scenarios where you want a controlled environment separate from the main thread, yet still 
 * recognized as a special "AppThread" for synchronization or event handling purposes.
 */
@interface FAppThread : NSThread

/**
 * @brief Initializes the AppThread instance with no specific target or selector.
 * @return A newly initialized FAppThread instance.
 *
 * This initializer sets up the thread’s internal state to represent the application’s 
 * designated thread. Use `-start` to actually begin the thread’s execution path defined in `-main`.
 */
- (id)init;

/**
 * @brief Initializes the AppThread with a given target, selector, and argument.
 * @param Target The object to which the specified selector message is sent.
 * @param Selector The selector to invoke on the target when the thread starts.
 * @param Argument The single argument passed to the selector when it is invoked.
 * @return A newly initialized FAppThread instance configured to run the specified 
 * selector on the target object when started.
 *
 * This method mirrors NSThread’s target-selector initialization pattern, allowing you 
 * to specify what the thread should do when it begins executing. Once started, the thread 
 * will execute the given method on the provided target object.
 */
- (id)initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument;

/**
 * @brief The main entry point for the AppThread.
 *
 * This method is automatically invoked when the thread is started and is intended to 
 * represent the core execution loop of the application’s designated thread. Override 
 * this method to provide custom run loops, event handling, or any other logic that 
 * should run on the AppThread.
 */
- (void)main;

/**
 * @brief Cleans up resources associated with the AppThread instance.
 *
 * Called when the thread object is deallocated, this method can be used to release 
 * any resources or perform any necessary cleanup related to the AppThread’s lifecycle.
 */
- (void)dealloc;

@end

/**
 * @brief A lightweight Objective-C wrapper around a custom CFRunLoopSourceContext-based run loop source.
 *
 * This class provides an Objective-C interface for creating, scheduling, and managing a custom run loop
 * source that is backed by a CFRunLoopSourceContext. By encapsulating the lifetime and interaction with 
 * the underlying Core Foundation constructs, it simplifies working with run loops in Objective-C code.
 *
 * Use this class to integrate your custom event source into a run loop, allowing it to receive 
 * asynchronous callbacks and trigger associated actions. You can schedule the source on a specific 
 * run loop and mode using `-scheduleOn:inMode:`, remove it with `-cancelFrom:inMode:`, and manually 
 * invoke its behavior using `-perform`. Internally, this class handles memory management, ensuring 
 * that the run loop source is properly created and cleaned up.
 */
@interface FRunLoopSource : NSObject
{
@private

    /**
     * @brief The context object backing the run loop source.
     *
     * This context contains function pointers and user data that define how the run loop source
     * interacts with the system, such as callbacks for when the source is fired or when it needs
     * to be invalidated.
     */
    FRunLoopSourceContext* Context;

    /**
     * @brief A reference to the run loop on which this source is currently scheduled.
     *
     * Once scheduled, the run loop uses this source to handle specified events or callbacks. 
     * If `nil`, it means the source is currently not scheduled on any run loop.
     */
    CFRunLoopRef ScheduledRunLoop;

    /**
     * @brief The run loop mode under which this source has been added.
     *
     * Run loops may operate in different modes, which can affect how and when various sources
     * fire. This property helps ensure that the source is only triggered under the appropriate mode.
     */
    CFStringRef  ScheduledMode;
}

/**
 * @brief Initializes the object with a given run loop source context.
 * @param InContext The run loop source context that contains callbacks and user data defining the behavior of this run loop source.
 * @return An initialized instance of FRunLoopSource.
 */
- (id)initWithContext:(FRunLoopSourceContext*)InContext;

/**
 * @brief Releases resources associated with the run loop source.
 *
 * This includes invalidating the source (if necessary) and freeing any allocated memory.
 * After this method is called, the object should no longer be used.
 */
- (void)dealloc;

/**
 * @brief Schedules the run loop source on the specified run loop and mode.
 * @param InRunLoop A reference to the run loop where the source should be added.
 * @param InMode The run loop mode under which the source should operate.
 *
 * Once scheduled, the run loop source will be triggered according to the callbacks defined in `Context`
 * whenever events occur that match the run loop mode and timing constraints.
 */
- (void)scheduleOn:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode;

/**
 * @brief Cancels and removes the run loop source from the specified run loop and mode.
 * @param InRunLoop A reference to the run loop from which the source should be removed.
 * @param InMode The run loop mode the source was previously scheduled in.
 *
 * After calling this, the run loop source will no longer receive events or callbacks from the
 * specified run loop and mode.
 */
- (void)cancelFrom:(CFRunLoopRef)InRunLoop inMode:(CFStringRef)InMode;

/**
 * @brief Manually triggers the run loop source’s callback.
 *
 * Calling this method forces the run loop source to perform its associated action or callback
 * without waiting for the run loop to schedule it.
 */
- (void)perform;

@end

/**
 * @brief Represents a task to be executed within specific run loop modes.
 *
 * The FRunLoopTask struct encapsulates a block of code (dispatch_block_t) that is intended to be executed
 * within one or more specified run loop modes. It retains a reference to the run loop modes to ensure
 * that the task is executed in the correct context and manages the memory of the block appropriately.
 */
struct FRunLoopTask
{
    /**
     * @brief Constructs a new FRunLoopTask with specified run loop modes and a block to execute.
     * @param InRunLoopModes An NSArray containing the run loop modes in which this task should be executed. Each mode is represented as an NSString.
     * @param InBlock A dispatch_block_t representing the block of code to execute when the task is performed.
     * @note This constructor retains the provided run loop modes and copies the provided block to manage its memory.
     */
    FRunLoopTask(NSArray* InRunLoopModes, dispatch_block_t InBlock)
        : RunLoopModes([InRunLoopModes retain]) // Retain the NSArray to maintain ownership
        , Block(Block_copy(InBlock))            // Copy the block to ensure it stays valid
    {
    }

    /**
     * @brief Destructs the FRunLoopTask, releasing retained resources.
     *
     * This destructor releases the copied block and the retained run loop modes to prevent memory leaks.
     */
    ~FRunLoopTask()
    {
        Block_release(Block);   // Release the copied block
        [RunLoopModes release]; // Release the retained NSArray
    }

    /**
     * @brief The run loop modes in which this task should be executed.
     *
     * An NSArray of NSStrings representing the run loop modes. These modes determine the contexts
     * in which the task's block will be performed. For example, common run loop modes include
     * `kCFRunLoopDefaultMode` and `NSModalPanelRunLoopMode`.
     */
    NSArray* RunLoopModes;

    /**
     * @brief The block of code to execute when the task is performed.
     *
     * A `dispatch_block_t` representing the block of code that will be executed. This block is
     * copied during initialization to ensure it remains valid for the lifetime of the task.
     */
    dispatch_block_t Block;
};

/**
 * @brief A manager class for a CFRunLoop associated with a particular thread.
 *
 * FRunLoopSourceContext is responsible for setting up run loop sources and managing
 * scheduled tasks for either the main thread or a designated application thread.
 * This class maintains a mapping of run loop modes to sources, allowing tasks to be
 * scheduled and executed on the associated run loop. It integrates with a custom
 * reference counting system and ensures proper cleanup of associated resources.
 */
class FRunLoopSourceContext : public FRefCounted
{
public:

    /**
     * @brief Get the context object associated with the main thread’s run loop.
     * @return A reference to the main thread's run loop context.
     * @note The main thread’s run loop must be registered beforehand.
     */
    static FRunLoopSourceContext& GetMainThreadContext();

    /**
     * @brief Get the context object associated with the application thread’s run loop.
     * @return A reference to the application thread's run loop context.
     * @note The application thread’s run loop must be registered beforehand.
     */
    static FRunLoopSourceContext& GetAppThreadContext();

    /**
     * @brief Register the current run loop (which must be the main thread’s) as the main thread’s run loop context.
     * @return true if successful.
     */
    static bool RegisterMainThreadRunLoop();

    /**
     * @brief Register the current run loop (which must be the application thread’s) as the application thread’s run loop context.
     * @return true if successful.
     */
    static bool RegisterAppThreadRunLoop();

    /**
     * @brief Constructor, initializes the run loop context with a given CFRunLoop.
     * @param InRunLoop The CFRunLoop that this context manages.
     */
    FRunLoopSourceContext(CFRunLoopRef InRunLoop);

    /**
     * @brief Destructor, cleans up sources and tasks, and deregisters this context if it matches the global main or application thread contexts.
     */
    ~FRunLoopSourceContext();

    /**
     * @brief Registers a run loop mode. If a source is not already associated with this mode, one is created and attached to the run loop.
     * @param InRunLoopMode The run loop mode to register.
     */
    void RegisterForMode(CFStringRef InRunLoopMode);

    /**
     * @brief Schedules a block (task) to be executed in one or more run loop modes.
     * @param Block The block to execute.
     * @param InModes The run loop modes in which this task should be run.
     */
    void ScheduleBlock(dispatch_block_t Block, NSArray* InModes);

    /**
     * @brief Executes all queued tasks that are eligible to run in the given run loop mode. This method extracts all queued tasks and repeatedly
     * executes any tasks that apply to the current mode until no more applicable tasks remain.
     * @param InRunLoopMode The current run loop mode in which tasks should be executed.
     */
    void Execute(CFStringRef InRunLoopMode);

    /**
     * @brief Runs the run loop in a specified mode until no input sources remain or a specific event occurs (may return immediately if none are pending).
     * @param RunMode The mode in which to run the run loop.
     */
    void RunInMode(CFStringRef RunMode);

    /**
     * @brief Wakes up the run loop, causing it to process pending events and tasks.
     */
    void WakeUp();

private:

    /** 
     * @brief Helper callback used with CFDictionaryApplyFunction to remove and cleanup sources when destructing this context.
     */
    static void Destroy(const void* Key, const void* Value, void* Context);

    /**
     * @brief Helper callback used with CFDictionaryApplyFunction to signal all sources, prompting them to perform tasks if ready.
     */
    static void Signal(const void* Key, const void* Value, void* Context);

    /**
     * @brief CFRunLoop callback that is invoked when a source is scheduled in a particular mode.
     */
    static void Schedule(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode);

    /**
     * @brief CFRunLoop callback that is invoked when a source is canceled from a particular mode.
     */
    static void Cancel(void* Info, CFRunLoopRef InRunLoop, CFRunLoopMode InRunLoopMode);

    /**
     * @brief CFRunLoop callback that performs the run loop source’s associated work.
     */
    static void Perform(void* Info);

private:

    /** @brief The managed run loop. */
    CFRunLoopRef RunLoop;

    /** @brief Maps run loop modes to CFRunLoopSourceRefs. */
    CFMutableDictionaryRef SourceAndModeDictionary;

    /** @brief Queue of pending tasks to execute. */
    TQueue<FRunLoopTask*> Tasks;

    /** @brief Synchronization primitive for Tasks. */
    FCriticalSection TasksCS;

    // Global pointers to main and application thread contexts.
    static FRunLoopSourceContext* MainThreadContext;
    static FRunLoopSourceContext* AppThreadContext;
};

/**
 * @brief Manages the main and application threads' run loops on macOS.
 *
 * The FMacThreadManager class provides an interface for setting up, managing, and executing tasks
 * on both the main thread and a dedicated application thread. It allows other threads to schedule
 * blocks of code to be executed on these threads' run loops, facilitating thread-safe operations
 * and inter-thread communication.
 */

class CORE_API FMacThreadManager
{
public:

    /**
     * @brief Sets up and starts the AppThread, initializing the main and AppThread's run loops.
     * @param Delegate The target object for the AppThread.
     * @param AppThreadEntry The selector to invoke on the AppThread when it starts.
     * @return true if the setup is successful, false otherwise.
     * @note This method disables sudden termination, registers the main thread's run loop, and initializes the AppThread with the provided delegate and selector.
     */
    static bool SetupAppThread(id Delegate, SEL AppThreadEntry);
    
    /**
     * @brief Releases and shuts down the AppThread.
     * @note This method releases the global reference to the AppThread, effectively shutting it down.
     */
    static void ShutdownAppThread();
    
    /**
     * @brief Runs the AppThread's run loop and ensures there are no pending events.
     * @param bUntilEmpty If true, the run loop will continue running until there are no more events to process.
     * @note This method processes events on the AppThread's run loop, optionally running until all events are handled.
     */
    static void PumpMessagesAppThread(bool bUntilEmpty);
    
    /**
     * @brief Executes a block of code on the main thread.
     * @param Block The block of code to execute.
     * @param WaitMode The run loop mode to use while waiting.
     * @param WaitForCompletion If true, the calling thread will wait until the block has been executed.
     * @note This method schedules the provided block to be executed on the main thread's run loop.
     */
    static void ExecuteOnMainThread(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);
    
    /**
     * @brief Executes a block of code on the application thread.
     * @param Block The block of code to execute.
     * @param WaitMode The run loop mode to use while waiting.
     * @param WaitForCompletion If true, the calling thread will wait until the block has been executed.
     * @note This method schedules the provided block to be executed on the AppThread's run loop.
     */
    static void ExecuteOnAppThread(dispatch_block_t Block, NSString* WaitMode, bool WaitForCompletion);
    
    /**
     * @brief Executes a block of code on the main thread and waits for a return value.
     * @tparam ReturnType The type of the value returned by the block.
     * @param Block The block of code to execute, which returns a value of type ReturnType.
     * @param WaitMode The run loop mode to use while waiting.
     * @return The value returned by the executed block.
     * @note This method schedules the provided block to be executed on the main thread and waits for its completion, returning the result produced by the block.
     */
    template<typename ReturnType>
    static inline ReturnType ExecuteOnMainThreadAndReturn(ReturnType (^Block)(void), NSString* WaitMode)
    {
        __block ReturnType ReturnValue;
        ExecuteOnMainThread(^
        {
            ReturnValue = Block();
        }, WaitMode, true);
        
        return ReturnValue;
    }
    
    /**
     * @brief Executes a block of code on the application thread and waits for a return value.
     * @tparam ReturnType The type of the value returned by the block.
     * @param Block The block of code to execute, which returns a value of type ReturnType.
     * @param WaitMode The run loop mode to use while waiting.
     * @return The value returned by the executed block.
     * @note This method schedules the provided block to be executed on the AppThread and waits for its completion, returning the result produced by the block.
     */
    template<typename ReturnType>
    static inline ReturnType ExecuteOnAppThreadAndReturn(ReturnType (^Block)(void), NSString* WaitMode)
    {
        __block ReturnType ReturnValue;
        ExecuteOnAppThread(^
        {
            ReturnValue = Block();
        }, WaitMode, true);
        
        return ReturnValue;
    }

private:

    /**
     * @brief Executes a block of code on the specified run loop context.
     * @param SourceContext The run loop context (main or application thread).
     * @param Block The block of code to execute.
     * @param WaitMode The run loop mode to use while waiting.
     * @param bWaitUntilFinished If true, the calling thread will wait until the block has been executed.
     * @note This private method handles the execution logic for both main and application threads.
     */
    static void ExecuteOnThread(FRunLoopSourceContext& SourceContext, dispatch_block_t Block, NSString* WaitMode, bool bWaitUntilFinished);
};
