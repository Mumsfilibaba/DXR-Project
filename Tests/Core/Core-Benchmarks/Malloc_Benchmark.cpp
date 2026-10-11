#include "Malloc_Benchmark.h"

#if RUN_MALLOC_BENCHMARKS
#include "TestCommon/Benchmark.h"

#include <Core/Memory/Memory.h>
#include <Core/Memory/MemoryProfiler.h>

static constexpr int32 GBlocksPerIteration = 64;

static int64 RunMallocFreeCycles(uint32 TestCount, uint32 Iterations)
{
    void* Blocks[GBlocksPerIteration];

    FClock Clock;
    for (uint32 Test = 0; Test < TestCount; ++Test)
    {
        FScopedClock ScopedClock(Clock);
        for (uint32 Iteration = 0; Iteration < Iterations; ++Iteration)
        {
            for (int32 Index = 0; Index < GBlocksPerIteration; ++Index)
            {
                Blocks[Index] = Memory::Malloc(16 + (Index % 16) * 64);
            }

            for (int32 Index = 0; Index < GBlocksPerIteration; ++Index)
            {
                Memory::Free(Blocks[Index]);
            }
        }
    }

    return Clock.GetTotalDuration() / TestCount;
}

void Malloc_Benchmark()
{
    LOG_INFO("\nBenchmark (Memory::Malloc / Memory::Free)");

    const uint32 TestCount  = Benchmark::ScaleCount(20, 2);
    const uint32 Iterations = Benchmark::ScaleCount(2000, 200);
    const uint32 Operations = Iterations * GBlocksPerIteration;
    LOG_INFO("\n%d blocks of 16 to 976 bytes, allocated then freed (Iterations=%u, TestCount=%u)", GBlocksPerIteration, Iterations, TestCount);

#if ENABLE_MEMORY_PROFILER
    FMemoryProfiler& Profiler = FMemoryProfiler::Get();

    Profiler.SetTrackingEnabled(false);
    Benchmark::Report("Tracking off", RunMallocFreeCycles(TestCount, Iterations), Operations);

    Profiler.SetTrackingEnabled(true);
    Benchmark::Report("Tracking live", RunMallocFreeCycles(TestCount, Iterations), Operations);
#else
    Benchmark::Report("Profiler compiled out", RunMallocFreeCycles(TestCount, Iterations), Operations);
#endif
}
#endif
