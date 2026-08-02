#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Core/Threading/ThreadManager.h>
#include <Core/Tasks/TaskGraph.h>

#include "TestCommon/TestHarness.h"

#include "Array_Benchmark.h"
#include "TaskGraph_Benchmark.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

    // The harness is used here for its logging and output-device setup only; benchmarks
    // report timings rather than pass/fail, so there is nothing to tally.
    TestHarness::Initialize("BenchmarkResults_Core.log");
    LOG_INFO("=== Core Benchmarks ===");

    FThreadManager::Initialize();

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

#if RUN_TARRAY_BENCHMARKS
    TArray_Benchmark();
#endif

#if RUN_TASKGRAPH_BENCHMARKS
    TaskGraph_Benchmark();
#endif

    FTaskGraph::Release();
    FThreadManager::Release();

    LOG_INFO("=== Core Benchmarks complete ===");
    TestHarness::Shutdown();
    return 0;
}
