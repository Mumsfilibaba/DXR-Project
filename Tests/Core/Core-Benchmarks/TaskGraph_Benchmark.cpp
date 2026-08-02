#include "TaskGraph_Benchmark.h"

#if RUN_TASKGRAPH_BENCHMARKS
#include <Core/Tasks/Tasks.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Tasks/TaskHandle.h>
#include <Core/Threading/Atomic/AtomicInt.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Platform/PlatformTime.h>

// ------------------------------------------------------------------------------------------------
// Throughput of empty tasks driven through the graph. Prints tasks/sec so a change to the
// scheduler can be compared against the central-queue baseline.
// ------------------------------------------------------------------------------------------------

void TaskGraph_Benchmark()
{
    constexpr int32 NumTasks = 200000;

    AtomicInt32 Completed(0);

    const uint64 Frequency = FPlatformTime::QueryPerformanceFrequency();
    const uint64 Start     = FPlatformTime::QueryPerformanceCounter();

    FTaskHandle Root = Tasks::Launch("BenchRoot", [&]()
    {
        for (int32 Index = 0; Index < NumTasks; ++Index)
        {
            Tasks::Async([&Completed]()
            {
                Completed.Increment();
            });
        }
    });

    Root.Wait();

    while (Completed.Load() < NumTasks)
    {
        FPlatformThreadMisc::Pause();
    }

    const uint64 End = FPlatformTime::QueryPerformanceCounter();

    const double Seconds     = (Frequency > 0) ? (static_cast<double>(End - Start) / static_cast<double>(Frequency)) : 0.0;
    const double TasksPerSec = (Seconds > 0.0) ? (static_cast<double>(NumTasks) / Seconds) : 0.0;

    LOG_INFO("[BENCHMARK] %d tasks in %.3f ms (%.3f M tasks/sec) across %d workers",
        NumTasks, Seconds * 1000.0, TasksPerSec / 1.0e6, FTaskGraph::Get().GetNumAnyThreadWorkers());
}

#endif // RUN_TASKGRAPH_BENCHMARKS
