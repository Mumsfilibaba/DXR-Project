#include "MemoryStack_Benchmark.h"

#if RUN_MEMORYSTACK_BENCHMARKS
#include "TestCommon/Benchmark.h"

#include <Core/Memory/MemoryPagePool.h>
#include <Core/Memory/MemoryStack.h>
#include <Core/Misc/ConsoleManager.h>

static constexpr int32 GAllocationSize = 1024;
static constexpr int32 GPagesPerCycle  = 4;

static int64 RunAllocateAndResetCycles(uint32 TestCount, uint32 Iterations)
{
    const int32 NumAllocations = (FMemoryStack::PageSize / GAllocationSize) * GPagesPerCycle;

    FClock Clock;
    for (uint32 Test = 0; Test < TestCount; ++Test)
    {
        FScopedClock ScopedClock(Clock);
        for (uint32 Iteration = 0; Iteration < Iterations; ++Iteration)
        {
            FMemoryStack Stack;
            for (int32 Index = 0; Index < NumAllocations; ++Index)
            {
                Stack.PushBytes(GAllocationSize, 16);
            }
        }
    }

    return Clock.GetTotalDuration() / TestCount;
}

static void SetPoolingEnabled(IConsoleVariable* Variable, FMemoryPagePool& Pool, bool bEnabled)
{
    Variable->SetAsBool(bEnabled, EConsoleVariableFlags::SetByCode);

    Pool.Tick();
    Pool.Flush();
}

void MemoryStack_Benchmark()
{
    LOG_INFO("\nBenchmark (FMemoryStack)");

    IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable("Memory.EnableStackPagePooling");
    if (!Variable)
    {
        LOG_ERROR("Memory.EnableStackPagePooling was not found, skipping the benchmark");
        return;
    }

    FMemoryPagePool& Pool = FMemoryPagePool::Get();

    const uint32 TestCount  = Benchmark::ScaleCount(20, 2);
    const uint32 Iterations = Benchmark::ScaleCount(2000, 200);
    LOG_INFO("\nAllocate and reset (Iterations=%u, TestCount=%u)", Iterations, TestCount);

    SetPoolingEnabled(Variable, Pool, false);
    Benchmark::Report("Page pooling off", RunAllocateAndResetCycles(TestCount, Iterations), Iterations);

    SetPoolingEnabled(Variable, Pool, true);
    Benchmark::Report("Page pooling on", RunAllocateAndResetCycles(TestCount, Iterations), Iterations);

    Pool.Flush();
}
#endif
