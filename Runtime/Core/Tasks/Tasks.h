#pragma once
#include "Core/Core.h"
#include "Core/Tasks/TaskTypes.h"
#include "Core/Tasks/TaskHandle.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Time/Timespan.h"

// Convenience for enqueueing work on the render thread.
#define ENQUEUE_RENDER_COMMAND(Name, Body) Tasks::LaunchOnRenderThread(#Name, Body)

// Thread-affinity asserts. Compile out in release like CHECK.
#define CHECK_MAIN_THREAD()   CHECK(Tasks::IsInMainThread())
#define CHECK_RENDER_THREAD() CHECK(Tasks::IsInRenderThread())
#define CHECK_RHI_THREAD()    CHECK(Tasks::IsInRHIThread())

struct CORE_API Tasks : public FNonConstructible
{
    /**
     * @brief Launches a task on the given lane with optional prerequisites.
     * @param DebugName Profiler scope name for the task. It is stored by pointer and read when the task executes, which may be deferred well past this call.
     * @return Returns a handle to the task's completion.
     */
    template<typename LambdaType>
    static FTaskHandle Launch(const CHAR* DebugName, LambdaType&& Body, ENamedThread::Type Thread = ENamedThread::AnyThread, ETaskPriority::Type Priority = ETaskPriority::Normal, TArrayView<const FTaskHandle> Prerequisites = {})
    {
        return LaunchImpl(DebugName, TFunction<void()>(Forward<LambdaType>(Body)), Thread, Priority, Prerequisites);
    }

    template<typename LambdaType>
    static FTaskHandle LaunchOnMainThread(const CHAR* DebugName, LambdaType&& Body, TArrayView<const FTaskHandle> Prerequisites = {})
    {
        return Launch(DebugName, Forward<LambdaType>(Body), ENamedThread::MainThread, ETaskPriority::Normal, Prerequisites);
    }

    template<typename LambdaType>
    static FTaskHandle LaunchOnRenderThread(const CHAR* DebugName, LambdaType&& Body, TArrayView<const FTaskHandle> Prerequisites = {})
    {
        return Launch(DebugName, Forward<LambdaType>(Body), ENamedThread::RenderThread, ETaskPriority::Normal, Prerequisites);
    }

    template<typename LambdaType>
    static FTaskHandle LaunchOnRHIThread(const CHAR* DebugName, LambdaType&& Body, TArrayView<const FTaskHandle> Prerequisites = {})
    {
        return Launch(DebugName, Forward<LambdaType>(Body), ENamedThread::RHIThread, ETaskPriority::Normal, Prerequisites);
    }

    /**
     * @brief Fire-and-forget shortcut for "just run this on a worker". Equivalent to 
     * Launch("Async", Lambda, ENamedThread::AnyThread, Priority). The returned handle
     * may be ignored, the task runs to completion regardless.
     */
    template<typename LambdaType>
    static FTaskHandle Async(LambdaType&& Lambda, ETaskPriority::Type Priority = ETaskPriority::Normal)
    {
        return Launch("Async", Forward<LambdaType>(Lambda), ENamedThread::AnyThread, Priority);
    }

    /** @brief Blocks until all of the given tasks have completed. */
    static void Wait(TArrayView<const FTaskHandle> InTasks, FTimespan Timeout = FTimespan::Infinity());

    /** @brief Drains the MainThread lane on the calling (main) thread. */
    static void ProcessMainThreadTasks();

    /** @return Returns true if the calling thread is the main thread. */
    static bool IsInMainThread();

    /**
     * @return Returns true if the calling thread is the render thread. When the render thread is 
     * disabled, render work runs inline on the main thread, so this returns true there too.
     */
    static bool IsInRenderThread();

    /** @return Returns true if the calling thread is the RHI thread. */
    static bool IsInRHIThread();

    /**
     * @brief Runs Body(Index) for Index in [0, Count) across the worker pool.
     * The calling thread participates in one chunk; the call returns once every element has run.
     * Body must be safe to invoke concurrently on distinct indices.
     */
    template<typename FuncType>
    static void ParallelFor(int32 Count, FuncType&& Body, int32 MinPerJob = 1, ETaskPriority::Type Priority = ETaskPriority::Normal);

    /**
     * @brief Range-based variant of ParallelFor that invokes BodyRange(Begin, End) per chunk.
     */
    template<typename FuncType>
    static void ParallelForRange(int32 Count, FuncType&& BodyRange, int32 MinPerJob = 1, ETaskPriority::Type Priority = ETaskPriority::Normal);

private:
    static FTaskHandle LaunchImpl(const CHAR* DebugName, TFunction<void()>&& Body, ENamedThread::Type Thread, ETaskPriority::Type Priority, TArrayView<const FTaskHandle> Prerequisites);
};
