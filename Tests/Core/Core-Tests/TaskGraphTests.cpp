#include "TaskGraphTests.h"

#include <Core/Tasks/Tasks.h>
#include <Core/Tasks/ParallelFor.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Tasks/TaskHandle.h>
#include <Core/Tasks/TaskGraphStats.h>
#include <Core/Threading/Atomic/AtomicInt.h>
#include <Core/Threading/ScopedLock.h>
#include <Core/Platform/CriticalSection.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Platform/PlatformTime.h>
#include <Core/Containers/Array.h>
#include <Core/Time/Timespan.h>

#include "TestCommon/TestMacros.h"

#define TG_CHECK(Condition) \
    if (!(Condition)) \
    { \
        LOG_ERROR("[TEST FAILED] Condition='%s'", #Condition); \
        return false; \
    }

// ------------------------------------------------------------------------------------------------
// Launch + Wait completes
// ------------------------------------------------------------------------------------------------

static bool Test_LaunchAndWait()
{
    AtomicInt32 Ran(0);

    FTaskHandle Handle = Tasks::Launch("LaunchAndWait", [&Ran]()
    {
        Ran.Increment();
    });

    Handle.Wait();

    TG_CHECK(Handle.IsComplete());
    TG_CHECK(Ran.Load() == 1);
    return true;
}

// ------------------------------------------------------------------------------------------------
// Prerequisite chain A -> B -> C runs in order
// ------------------------------------------------------------------------------------------------

static bool Test_PrerequisiteChain()
{
    AtomicInt32 Order(0);
    AtomicInt32 OrderA(-1);
    AtomicInt32 OrderB(-1);
    AtomicInt32 OrderC(-1);

    FTaskHandle TaskA = Tasks::Launch("A", [&]()
    {
        OrderA.Store(Order.Increment());
    });
    
    FTaskHandle TaskB = Tasks::Launch("B", [&]()
    {
        OrderB.Store(Order.Increment());
    }, ENamedThread::AnyThread, ETaskPriority::Normal, { TaskA });

    FTaskHandle TaskC = Tasks::Launch("C", [&]()
    {
        OrderC.Store(Order.Increment());
    }, ENamedThread::AnyThread, ETaskPriority::Normal, { TaskB });

    TaskC.Wait();

    TG_CHECK(OrderA.Load() == 1);
    TG_CHECK(OrderB.Load() == 2);
    TG_CHECK(OrderC.Load() == 3);
    return true;
}

// ------------------------------------------------------------------------------------------------
// Named-thread FIFO: tasks on a single named lane execute in submit order
// ------------------------------------------------------------------------------------------------

static bool Test_NamedThreadFIFO()
{
    constexpr int32 NumTasks = 100;

    FCriticalSection ExecutionOrderCS;
    TArray<int32>    ExecutionOrder;

    FTaskHandle LastHandle;
    for (int32 Index = 0; Index < NumTasks; ++Index)
    {
        LastHandle = Tasks::LaunchOnRHIThread("FIFO", [&ExecutionOrderCS, &ExecutionOrder, Index]()
        {
            SCOPED_LOCK(ExecutionOrderCS);
            ExecutionOrder.Add(Index);
        });
    }

    LastHandle.Wait();

    TG_CHECK(ExecutionOrder.Size() == NumTasks);

    bool bInOrder = true;
    for (int32 Index = 0; Index < ExecutionOrder.Size(); ++Index)
    {
        if (ExecutionOrder[Index] != Index)
        {
            bInOrder = false;
            break;
        }
    }

    TG_CHECK(bInOrder);
    return true;
}

// ------------------------------------------------------------------------------------------------
// ParallelFor runs the body exactly Count times
// ------------------------------------------------------------------------------------------------

static bool Test_ParallelFor()
{
    constexpr int32 Count = 10000;

    AtomicInt32 NumInvocations(0);
    AtomicInt64 IndexSum(0);

    Tasks::ParallelFor(Count, [&](int32 Index)
    {
        NumInvocations.Increment();
        IndexSum.Add(Index);
    });

    const int64 ExpectedSum = (static_cast<int64>(Count) * (Count - 1)) / 2;

    TG_CHECK(NumInvocations.Load() == Count);
    TG_CHECK(IndexSum.Load() == ExpectedSum);
    return true;
}

// ------------------------------------------------------------------------------------------------
// Main-thread queueing + deadlock-safe Wait: a worker waits on a main-thread task while the main
// thread pumps the main-thread queue.
// ------------------------------------------------------------------------------------------------

static bool Test_MainThreadPumpNoDeadlock()
{
    AtomicInt32 MainTaskRan(0);

    FTaskHandle WorkerHandle = Tasks::Launch("Worker", [&]()
    {
        FTaskHandle MainHandle = Tasks::LaunchOnMainThread("MainWork", [&]()
        {
            MainTaskRan.Increment();
        });

        // This worker blocks on a main-thread task; it must not deadlock because the main thread
        // pumps the main-thread queue below.
        MainHandle.Wait();
    });

    // Main thread pumps the main-thread queue until the worker finishes.
    while (!WorkerHandle.IsComplete())
    {
        Tasks::ProcessMainThreadTasks();
        FPlatformThreadMisc::Pause();
    }

    TG_CHECK(MainTaskRan.Load() == 1);
    TG_CHECK(WorkerHandle.IsComplete());
    return true;
}

// ------------------------------------------------------------------------------------------------
// Stress: a root task fans out a large number of child tasks. Because the children are launched
// from inside a worker, they land on that worker's own deque and the remaining idle workers must
// steal them. Verifies all tasks complete and (with >1 worker) that work-stealing actually engaged.
// ------------------------------------------------------------------------------------------------

static bool Test_WorkStealingStress()
{
    constexpr int32 NumChildTasks = 100000;

#if STATS_ENABLED
    // Stealing is opportunistic, so give it a few attempts before declaring it never engaged.
    constexpr int32 MaxAttempts   = 16;

    const int64 StealsBefore  = STAT_GET(STAT_TaskGraph_StealSuccesses);
    const bool  bExpectSteals = FTaskGraph::Get().GetNumAnyThreadWorkers() > 1;
    int64       StealsDelta   = 0;
#else
    // Without the steal counter there is nothing to retry for.
    constexpr int32 MaxAttempts   = 1;
#endif

    for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
    {
        AtomicInt32 Completed(0);

        FTaskHandle Root = Tasks::Launch("StressRoot", [&]()
        {
            for (int32 Index = 0; Index < NumChildTasks; ++Index)
            {
                Tasks::Async([&Completed]()
                {
                    Completed.Increment();
                });
            }
        });

        Root.Wait();

        // Spin until every child task has run.
        while (Completed.Load() < NumChildTasks)
        {
            FPlatformThreadMisc::Pause();
        }

        TG_CHECK(Completed.Load() == NumChildTasks);

    #if STATS_ENABLED
        StealsDelta = STAT_GET(STAT_TaskGraph_StealSuccesses) - StealsBefore;
        if (!bExpectSteals || StealsDelta > 0)
        {
            break;
        }
    #endif
    }

#if STATS_ENABLED
    if (bExpectSteals)
    {
        LOG_INFO("Work-stealing steals during stress: %lld", static_cast<int64>(StealsDelta));
        TG_CHECK(StealsDelta > 0);
    }
#endif

    return true;
}

bool TaskGraph_Test()
{
    LOG_INFO("=== Task Graph Tests ===");

    bool bResult = true;
    bResult = Test_LaunchAndWait()             && bResult;
    bResult = Test_PrerequisiteChain()         && bResult;
    bResult = Test_NamedThreadFIFO()           && bResult;
    bResult = Test_ParallelFor()               && bResult;
    bResult = Test_MainThreadPumpNoDeadlock()  && bResult;
    bResult = Test_WorkStealingStress()        && bResult;

    LOG_INFO(bResult ? "[TASK GRAPH TESTS SUCCEEDED]" : "[TASK GRAPH TESTS FAILED]");
    return bResult;
}
