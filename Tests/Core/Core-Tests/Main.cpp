#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Threading/ThreadManager.h>
#include <Core/Tasks/TaskGraph.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "TaskGraphTests.h"
#include "CommandLineTests.h"
#include "ConsoleManagerCommandLineTests.h"
#include "IniFileTests.h"
#include "JsonParserTests.h"
#include "JsonWriterTests.h"
#include "JsonArchiveTests.h"
#include "BlueNoiseGeneratorTests.h"
#include "PlatformTimeTests.h"
#include "MemoryStackTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

    TestHarness::Initialize("TestResults_Core.log");
    LOG_INFO("=== Core Tests ===");

    FThreadManager::Initialize();

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    RUN_TEST("TaskGraph", TaskGraph_Test());
    RUN_TEST("CommandLine", CommandLine_Test());
    RUN_TEST("ConsoleManagerCommandLine", ConsoleManagerCommandLine_Test());
    RUN_TEST("IniFile", IniFile_Test());
    RUN_TEST("JsonParser", JsonParser_Test());
    RUN_TEST("JsonWriter", JsonWriter_Test());
    RUN_TEST("JsonArchive", JsonArchive_Test());
    RUN_TEST("BlueNoiseGenerator", BlueNoiseGenerator_Test());
    RUN_TEST("PlatformTime", PlatformTime_Test());
    RUN_TEST("MemoryStack", MemoryStack_Test());

    FTaskGraph::Release();
    FThreadManager::Release();

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}
