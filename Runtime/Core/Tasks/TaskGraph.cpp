#include "Core/Tasks/TaskGraph.h"
#include "Core/Tasks/TaskWorker.h"
#include "Core/Tasks/TaskBase.h"
#include "Core/Tasks/TaskEvent.h"
#include "Core/Tasks/TaskGraphStats.h"
#include "Core/Containers/WorkStealingDeque.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Containers/String.h"
#include "Core/Math/Math.h"
#include "Core/Math/Random.h"

static TAutoConsoleVariable<int32> CVarNumWorkerThreads(
    "TaskGraph.NumWorkerThreads",
    "Number of anonymous worker threads in the task graph. -1 selects (NumProcessors - 2), clamped to a minimum of 2.",
    -1);

static TAutoConsoleVariable<bool> CVarEnableRenderThread(
    "TaskGraph.EnableRenderThread",
    "When false, tasks launched on the render thread run inline on the calling thread.",
    true);

static TAutoConsoleVariable<bool> CVarEnableRHIThread(
    "TaskGraph.EnableRHIThread",
    "When false, tasks launched on the RHI thread run inline on the calling thread.",
    true);

static constexpr int32 MIN_ANYTHREAD_WORKERS = 2;

static void AddPendingStat(ENamedThread::Type Lane, int64 Delta)
{
    UNREFERENCED_VARIABLE(Delta);

    switch (Lane)
    {
    case ENamedThread::AnyThread:
        STAT_ADD(STAT_TaskGraph_AnyThreadPending, Delta);
        break;
    case ENamedThread::MainThread:
        STAT_ADD(STAT_TaskGraph_MainThreadPending, Delta);
        break;
    case ENamedThread::RenderThread:
        STAT_ADD(STAT_TaskGraph_RenderThreadPending, Delta);
        break;
    case ENamedThread::RHIThread:
        STAT_ADD(STAT_TaskGraph_RHIThreadPending, Delta);
        break;
    default:
        break;
    }
}

FTaskGraph* FTaskGraph::TaskGraph = nullptr;

FTaskGraph::FTaskGraph()
    : AnyThreadSlots()
    , Workers()
    , NumAnyThreadWorkers(0)
    , bRenderThreadEnabled(true)
    , bRHIThreadEnabled(true)
{
}

FTaskGraph::~FTaskGraph()
{
}

bool FTaskGraph::Initialize()
{
    if (TaskGraph)
    {
        return true;
    }

    TaskGraph = new FTaskGraph();
    if (!TaskGraph->Startup())
    {
        TaskGraph->Shutdown();
        delete TaskGraph;

        TaskGraph = nullptr;
        return false;
    }

    return true;
}

void FTaskGraph::Release()
{
    if (TaskGraph)
    {
        TaskGraph->Shutdown();

        delete TaskGraph;
        TaskGraph = nullptr;
    }
}

bool FTaskGraph::Startup()
{
    // Allocate the current-worker TLS slot before any worker thread is created.
    if (!FTaskWorker::AllocateTLSSlot())
    {
        LOG_ERROR("[FTaskGraph] Failed to allocate current-worker TLS slot");
        return false;
    }

    for (int32 Index = 0; Index < ENamedThread::Count; ++Index)
    {
        Lanes[Index].LaneType        = static_cast<ENamedThread::Type>(Index);
        Lanes[Index].bInlineFallback = false;
    }

    bRenderThreadEnabled = CVarEnableRenderThread.GetValue();
    bRHIThreadEnabled    = CVarEnableRHIThread.GetValue();

    const int32 NumProcessors = static_cast<int32>(FPlatformThreadMisc::GetNumProcessors());

    int32 NumWorkers = CVarNumWorkerThreads.GetValue();
    if (NumWorkers <= 0)
    {
        NumWorkers = NumProcessors - 2;
    }

    NumWorkers = Math::Max(NumWorkers, MIN_ANYTHREAD_WORKERS);
    NumWorkers = Math::Min(NumWorkers, Math::Max(NumProcessors, MIN_ANYTHREAD_WORKERS));
    NumAnyThreadWorkers = NumWorkers;

    // Create the per-worker work-stealing slots *before* any worker thread starts, since a worker's
    // Run loop indexes AnyThreadSlots by its worker index.
    AnyThreadSlots.Reserve(NumAnyThreadWorkers);
    for (int32 Index = 0; Index < NumAnyThreadWorkers; ++Index)
    {
        FAnyThreadWorkerSlot Slot;
        for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
        {
            Slot.Deques[Priority] = new TWorkStealingDeque<FGraphTask*>();
        }

        Slot.Worker = nullptr;
        AnyThreadSlots.Add(Slot);
    }

    // Anonymous worker pool (AnyThread lane). Worker index == AnyThread slot index.
    for (int32 Index = 0; Index < NumAnyThreadWorkers; ++Index)
    {
        const String ThreadName = String::Printf("TaskWorker[%d]", Index);

        FTaskWorker* Worker = new FTaskWorker(this, ENamedThread::AnyThread, Index);
        if (!Worker->Initialize(*ThreadName))
        {
            LOG_ERROR("[FTaskGraph] Failed to create anonymous worker");
            delete Worker;
            return false;
        }

        AnyThreadSlots[Index].Worker = Worker;
        Workers.Add(Worker);
    }

    // Render-thread lane.
    if (bRenderThreadEnabled)
    {
        FTaskWorker* Worker = new FTaskWorker(this, ENamedThread::RenderThread, 0);
        if (!Worker->Initialize("RenderThread"))
        {
            LOG_ERROR("[FTaskGraph] Failed to create render-thread worker");
            delete Worker;
            return false;
        }

        Workers.Add(Worker);
    }
    else
    {
        Lanes[ENamedThread::RenderThread].bInlineFallback = true;
    }

    // RHI-thread lane.
    if (bRHIThreadEnabled)
    {
        FTaskWorker* Worker = new FTaskWorker(this, ENamedThread::RHIThread, 0);
        if (!Worker->Initialize("RHIThread"))
        {
            LOG_ERROR("[FTaskGraph] Failed to create RHI-thread worker");
            delete Worker;
            return false;
        }

        Workers.Add(Worker);
    }
    else
    {
        Lanes[ENamedThread::RHIThread].bInlineFallback = true;
    }

    STAT_SET(STAT_TaskGraph_TotalWorkers, Workers.Size());

    LOG_INFO("[FTaskGraph] Initialized with %d anonymous workers (render thread %s, RHI thread %s)", 
        NumAnyThreadWorkers, bRenderThreadEnabled ? "enabled" : "disabled", bRHIThreadEnabled ? "enabled" : "disabled");
    return true;
}

void FTaskGraph::Shutdown()
{
    // Signal every worker to stop, then join.
    for (FTaskWorker* Worker : Workers)
    {
        Worker->Stop();
    }

    for (FTaskWorker* Worker : Workers)
    {
        delete Worker;
    }

    Workers.Clear();

    // Drop any tasks that were queued but never executed.
    for (int32 Index = 0; Index < ENamedThread::Count; ++Index)
    {
        FNamedLane& Lane = Lanes[Index];
        SCOPED_LOCK(Lane.QueueCS);

        for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
        {
            for (FGraphTask* Task : Lane.Queue[Priority])
            {
                delete Task;
            }

            Lane.Queue[Priority].Clear();
        }

        Lane.IdleWorkers.Clear();
    }

    // Drain and free the per-worker deques (workers have all joined, so this is single-threaded).
    for (FAnyThreadWorkerSlot& Slot : AnyThreadSlots)
    {
        for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
        {
            TWorkStealingDeque<FGraphTask*>* Deque = Slot.Deques[Priority];
            if (Deque)
            {
                FGraphTask* Task = nullptr;
                while (Deque->PopBottom(Task))
                {
                    delete Task;
                }

                delete Deque;
                Slot.Deques[Priority] = nullptr;
            }
        }
    }

    AnyThreadSlots.Clear();

    // All workers are stopped, joined, and deleted; free the current-worker TLS slot.
    FTaskWorker::FreeTLSSlot();
}

FTaskHandle FTaskGraph::LaunchInternal(const CHAR* DebugName, TFunction<void()>&& Body, ENamedThread::Type Lane, ETaskPriority::Type Priority, TArrayView<const FTaskHandle> Prerequisites)
{
    FGraphTask* Task = new FGraphTask(DebugName, ::Move(Body), Lane, Priority);

    // Start with a single launch reference so the task cannot be scheduled while we are still wiring up its prerequisites below.
    Task->RemainingPrerequisites.Store(1);

    STAT_ADD(STAT_TaskGraph_TasksLaunched, 1);

    FTaskHandle Handle(Task->GetCompletionEvent());
    for (const FTaskHandle& Prerequisite : Prerequisites)
    {
        FTaskEvent* PrerequisiteEvent = Prerequisite.GetEvent();
        if (PrerequisiteEvent && !PrerequisiteEvent->IsComplete())
        {
            Task->RemainingPrerequisites.Increment();
            if (!PrerequisiteEvent->AddSubsequent(Task))
            {
                // Completed between the IsComplete check and AddSubsequent; undo the increment.
                Task->RemainingPrerequisites.Decrement();
            }
        }
    }

    // Release the launch reference. This schedules the task if all prerequisites already ran.
    OnPrerequisiteComplete(Task);
    return Handle;
}

void FTaskGraph::OnPrerequisiteComplete(FGraphTask* Task)
{
    if (Task->RemainingPrerequisites.Decrement() == 0)
    {
        SubmitReadyTask(Task);
    }
}

void FTaskGraph::SubmitReadyTask(FGraphTask* Task)
{
    const ENamedThread::Type LaneType = Task->GetLane();
    if (LaneType == ENamedThread::AnyThread)
    {
        AddPendingStat(ENamedThread::AnyThread, 1);

        FTaskWorker* Current = FTaskWorker::GetCurrentWorker();
        if (Current && Current->GetLane() == ENamedThread::AnyThread)
        {
            // Owner push onto this worker's own deque - zero contention, great locality.
            AnyThreadSlots[Current->GetWorkerIndex()].Deques[Task->GetPriority()]->PushBottom(Task);
        }
        else
        {
            // External producer: route through the shared central queue.
            SCOPED_LOCK(Lanes[ENamedThread::AnyThread].QueueCS);
            Lanes[ENamedThread::AnyThread].Queue[Task->GetPriority()].Add(Task);
        }

        WakeOneAnyThreadWorker();
        return;
    }

    FNamedLane& Lane = Lanes[LaneType];

    // Lane has no servicing worker (render thread disabled) - run synchronously on the caller.
    if (Lane.bInlineFallback)
    {
        Task->Execute();
        return;
    }

    FTaskWorker* WorkerToWake = nullptr;

    {
        SCOPED_LOCK(Lane.QueueCS);

        Lane.Queue[Task->GetPriority()].Add(Task);
        AddPendingStat(LaneType, 1);

        if (!Lane.IdleWorkers.IsEmpty())
        {
            WorkerToWake = Lane.IdleWorkers.Last();
            Lane.IdleWorkers.Pop();
            WorkerToWake->bIsIdle.Store(false);
        }
    }

    if (WorkerToWake)
    {
        WorkerToWake->Wake();
    }
}

void FTaskGraph::WakeOneAnyThreadWorker()
{
    FTaskWorker* WorkerToWake = nullptr;

    {
        FNamedLane& Lane = Lanes[ENamedThread::AnyThread];
        SCOPED_LOCK(Lane.QueueCS);

        if (!Lane.IdleWorkers.IsEmpty())
        {
            WorkerToWake = Lane.IdleWorkers.Last();
            Lane.IdleWorkers.Pop();
            WorkerToWake->bIsIdle.Store(false);
        }
    }

    if (WorkerToWake)
    {
        WorkerToWake->Wake();
    }
}

FGraphTask* FTaskGraph::PopFromNamedLane(FNamedLane& Lane)
{
    SCOPED_LOCK(Lane.QueueCS);

    for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
    {
        if (!Lane.Queue[Priority].IsEmpty())
        {
            FGraphTask* Task = Lane.Queue[Priority].First();
            Lane.Queue[Priority].RemoveAt(0);
            AddPendingStat(Lane.LaneType, -1);
            return Task;
        }
    }

    return nullptr;
}

FGraphTask* FTaskGraph::PopFromCentralAnyThread()
{
    FNamedLane& Lane = Lanes[ENamedThread::AnyThread];
    SCOPED_LOCK(Lane.QueueCS);

    for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
    {
        if (!Lane.Queue[Priority].IsEmpty())
        {
            FGraphTask* Task = Lane.Queue[Priority].First();
            Lane.Queue[Priority].RemoveAt(0);
            AddPendingStat(ENamedThread::AnyThread, -1);
            return Task;
        }
    }

    return nullptr;
}

FGraphTask* FTaskGraph::TryStealAnyThread(FTaskWorker* Worker)
{
    const int32 NumSlots = AnyThreadSlots.Size();
    if (NumSlots <= 0)
    {
        return nullptr;
    }

    const int32 SelfSlot = (Worker && Worker->GetLane() == ENamedThread::AnyThread) ? Worker->GetWorkerIndex() : -1;
    const int32 Attempts = 2 * NumSlots;

    // Default-seeds from the perf counter, so each worker thread gets a distinct sequence.
    static thread_local FRandom Random;

    for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
    {
        const int32 Victim = static_cast<int32>(Random.Rand() % static_cast<uint64>(NumSlots));
        if (Victim == SelfSlot)
        {
            continue;
        }

        FAnyThreadWorkerSlot& Slot = AnyThreadSlots[Victim];
        for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
        {
            STAT_ADD(STAT_TaskGraph_StealAttempts, 1);

            FGraphTask* Stolen = nullptr;
            if (Slot.Deques[Priority]->Steal(Stolen))
            {
                STAT_ADD(STAT_TaskGraph_StealSuccesses, 1);
                AddPendingStat(ENamedThread::AnyThread, -1);
                return Stolen;
            }
        }
    }

    return nullptr;
}

FGraphTask* FTaskGraph::TryAcquireWork(FTaskWorker* Worker)
{
    if (Worker->GetLane() != ENamedThread::AnyThread)
    {
        return PopFromNamedLane(Lanes[Worker->GetLane()]);
    }

    // Anonymous worker: own deque (LIFO) -> central queue (FIFO) -> steal from peers.
    FAnyThreadWorkerSlot& Self = AnyThreadSlots[Worker->GetWorkerIndex()];
    for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
    {
        FGraphTask* Task = nullptr;
        if (Self.Deques[Priority]->PopBottom(Task))
        {
            AddPendingStat(ENamedThread::AnyThread, -1);
            return Task;
        }
    }

    if (FGraphTask* Task = PopFromCentralAnyThread())
    {
        return Task;
    }

    return TryStealAnyThread(Worker);
}

void FTaskGraph::MarkWorkerIdle(FTaskWorker* Worker)
{
    FNamedLane& Lane = Lanes[Worker->GetLane()];
    SCOPED_LOCK(Lane.QueueCS);

    if (!Worker->bIsIdle.Load())
    {
        Worker->bIsIdle.Store(true);
        Lane.IdleWorkers.Add(Worker);
    }
}

void FTaskGraph::MarkWorkerActive(FTaskWorker* Worker)
{
    FNamedLane& Lane = Lanes[Worker->GetLane()];
    SCOPED_LOCK(Lane.QueueCS);

    if (Worker->bIsIdle.Load())
    {
        Worker->bIsIdle.Store(false);
        Lane.IdleWorkers.Remove(Worker);
    }
}

bool FTaskGraph::TryExecuteOneAnyThreadTask()
{
    FTaskWorker* Current = FTaskWorker::GetCurrentWorker();
    if (Current && Current->GetLane() == ENamedThread::AnyThread)
    {
        FAnyThreadWorkerSlot& Self = AnyThreadSlots[Current->GetWorkerIndex()];
        for (int32 Priority = 0; Priority < ETaskPriority::Count; ++Priority)
        {
            FGraphTask* Task = nullptr;
            if (Self.Deques[Priority]->PopBottom(Task))
            {
                AddPendingStat(ENamedThread::AnyThread, -1);
                Task->Execute();
                return true;
            }
        }
    }

    if (FGraphTask* Task = PopFromCentralAnyThread())
    {
        Task->Execute();
        return true;
    }

    if (FGraphTask* Task = TryStealAnyThread(Current))
    {
        Task->Execute();
        return true;
    }

    return false;
}

void FTaskGraph::ProcessMainThreadTasks()
{
    FNamedLane& Lane = Lanes[ENamedThread::MainThread];
    for (;;)
    {
        FGraphTask* Task = PopFromNamedLane(Lane);
        if (!Task)
        {
            break;
        }

        Task->Execute();
    }
}

void FTaskGraph::WaitForEvent(FTaskEvent* Event, FTimespan Timeout)
{
    if (!Event || Event->IsComplete())
    {
        return;
    }

    FTaskWorker* Worker = FTaskWorker::GetCurrentWorker();
    
    const bool bMainThread = FThreadManager::IsMainThread();
    if (bMainThread)
    {
        // Pump main-thread tasks (a render task may be waiting on a main-thread callback) and help with
        // anonymous work so the wait cannot deadlock the pool.
        while (!Event->IsComplete())
        {
            ProcessMainThreadTasks();
            
            if (Event->IsComplete())
            {
                break;
            }

            if (!TryExecuteOneAnyThreadTask())
            {
                Event->WaitUntilComplete(FTimespan::Milliseconds(1));
            }
        }

        return;
    }

    if (Worker)
    {
        while (!Event->IsComplete())
        {
            if (!TryExecuteOneAnyThreadTask())
            {
                Event->WaitUntilComplete(FTimespan::Milliseconds(50));
            }
        }

        return;
    }

    Event->WaitUntilComplete(Timeout);
}
