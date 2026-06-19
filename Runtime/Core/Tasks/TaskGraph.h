#pragma once
#include "Core/Core.h"
#include "Core/Tasks/TaskTypes.h"
#include "Core/Tasks/TaskHandle.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Function.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "Core/Time/Timespan.h"
#include "Core/Templates/Utility/NonCopyable.h"

class FTaskWorker;
class FGraphTask;
class FTaskEvent;

template<typename T>
class TWorkStealingDeque;

class CORE_API FTaskGraph : private FNonCopyable
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE bool IsInitialized()
    {
        return TaskGraph != nullptr;
    }

    static FORCEINLINE FTaskGraph& Get()
    {
        CHECK(FTaskGraph::IsInitialized());
        return *TaskGraph;
    }

public:

    /**
     * @brief Launches a task on the given lane with optional prerequisites.
     * @return Returns a handle to the task's completion event.
     */
    FTaskHandle LaunchInternal(const CHAR* DebugName, TFunction<void()>&& Body, ENamedThread::Type Lane, ETaskPriority::Type Priority, TArrayView<const FTaskHandle> Prerequisites);

    /** @brief Called for every subsequent released by a completing prerequisite. */
    void OnPrerequisiteComplete(FGraphTask* Task);

    /** @brief Drains the MainThread lane on the calling (main) thread. */
    void ProcessMainThreadTasks();

    /** @brief Blocks until Event completes, helping with work / pumping the main thread. */
    void WaitForEvent(FTaskEvent* Event, FTimespan Timeout);

    FORCEINLINE int32 GetNumAnyThreadWorkers() const
    {
        return NumAnyThreadWorkers;
    }

    FORCEINLINE bool IsRenderThreadEnabled() const
    {
        return bRenderThreadEnabled;
    }

    FORCEINLINE bool IsRHIThreadEnabled() const
    {
        return bRHIThreadEnabled;
    }

public:

    // Worker-facing API

    /** @brief Returns the next ready task for the worker without any idle side effects. */
    FGraphTask* TryAcquireWork(FTaskWorker* Worker);

    /** @brief Registers the worker as idle on its lane so submitters can wake it. */
    void MarkWorkerIdle(FTaskWorker* Worker);

    /** @brief Clears the worker's idle state. */
    void MarkWorkerActive(FTaskWorker* Worker);

private:
    struct FNamedLane
    {
        FCriticalSection     QueueCS;
        TArray<FGraphTask*>  Queue[ETaskPriority::Count];
        TArray<FTaskWorker*> IdleWorkers;
        ENamedThread::Type   LaneType        = ENamedThread::AnyThread;
        bool                 bInlineFallback = false;
    };

    struct FAnyThreadWorkerSlot
    {
        TWorkStealingDeque<FGraphTask*>* Deques[ETaskPriority::Count] = {};
        FTaskWorker*                     Worker                       = nullptr;
    };

    FTaskGraph();
    ~FTaskGraph();

    bool Startup();
    void Shutdown();

    void SubmitReadyTask(FGraphTask* Task);

    FGraphTask* PopFromNamedLane(FNamedLane& Lane);
    FGraphTask* PopFromCentralAnyThread();
    FGraphTask* TryStealAnyThread(FTaskWorker* Worker);
    bool        TryExecuteOneAnyThreadTask();

    void WakeOneAnyThreadWorker();

    FNamedLane                   Lanes[ENamedThread::Count];
    TArray<FAnyThreadWorkerSlot> AnyThreadSlots;
    TArray<FTaskWorker*>         Workers;
    int32                        NumAnyThreadWorkers;
    bool                         bRenderThreadEnabled;
    bool                         bRHIThreadEnabled;

    static FTaskGraph* TaskGraph;
};
