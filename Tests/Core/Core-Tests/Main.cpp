#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Threading/ThreadManager.h>
#include <Core/Tasks/TaskGraph.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "TaskGraphTests.h"
#include "CommandLineTests.h"
#include "ConsoleManagerCommandLineTests.h"
#include "IniFileTests.h"
#include "ConfigTests.h"
#include "JsonParserTests.h"
#include "JsonWriterTests.h"
#include "JsonArchiveTests.h"
#include "BlueNoiseGeneratorTests.h"
#include "PlatformTimeTests.h"
#include "PlatformMiscTests.h"
#include "MemoryStackTests.h"
#include "PlatformEventPoolTests.h"
#include "FileOutputDeviceTests.h"
#include "ProfilerTests.h"
#include "FileTests.h"
#include "GuidTests.h"
#include "PlatformProcessTests.h"
#include "ProjectDescriptorTests.h"
#include "PathsTests.h"
#include "CharTypeTests.h"
#include "UnicodePathTests.h"
#include "SocketAddressTests.h"
#include "PlatformSocketTests.h"
#include "NetworkSocketTests.h"
#include "RemoteConsoleProtocolTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunCoreTests()
{
    TestHarness::Initialize(TEXT("TestResults_Core.log"));
    LOG_INFO("=== Core Tests ===");

    FThreadManager::Initialize();

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    RUN_TEST(TEXT("TaskGraph"), TaskGraph_Test());
    RUN_TEST(TEXT("CommandLine"), CommandLine_Test());
    RUN_TEST(TEXT("ConsoleManagerCommandLine"), ConsoleManagerCommandLine_Test());
    RUN_TEST(TEXT("IniFile"), IniFile_Test());
    RUN_TEST(TEXT("Config"), Config_Test());
    RUN_TEST(TEXT("JsonParser"), JsonParser_Test());
    RUN_TEST(TEXT("JsonWriter"), JsonWriter_Test());
    RUN_TEST(TEXT("JsonArchive"), JsonArchive_Test());
    RUN_TEST(TEXT("BlueNoiseGenerator"), BlueNoiseGenerator_Test());
    RUN_TEST(TEXT("PlatformTime"), PlatformTime_Test());
    RUN_TEST(TEXT("PlatformMisc"), PlatformMisc_Test());
    RUN_TEST(TEXT("MemoryStack"), MemoryStack_Test());
    RUN_TEST(TEXT("PlatformEventPool"), PlatformEventPool_Test());
    RUN_TEST(TEXT("FileOutputDevice"), FileOutputDevice_Test());
    RUN_TEST(TEXT("Profiler"), Profiler_Test());
    RUN_TEST(TEXT("ProfilerGPUTraceSentinel"), ProfilerGPUTraceSentinel_Test());
    RUN_TEST(TEXT("File"), File_Test());
    RUN_TEST(TEXT("Guid"), Guid_Test());
    RUN_TEST(TEXT("PlatformProcess"), PlatformProcess_Test());
    RUN_TEST(TEXT("ProjectDescriptor"), ProjectDescriptor_Test());
    RUN_TEST(TEXT("Paths"), Paths_Test());
    RUN_TEST(TEXT("CharType"), CharType_Test());
    RUN_TEST(TEXT("UnicodePath"), UnicodePath_Test());
    RUN_TEST(TEXT("SocketAddress"), SocketAddress_Test());
    RUN_TEST(TEXT("PlatformSocket"), PlatformSocket_Test());
    RUN_TEST(TEXT("NetworkSocket"), NetworkSocket_Test());
    RUN_TEST(TEXT("RemoteConsoleProtocol"), RemoteConsoleProtocol_Test());

    FTaskGraph::Release();
    FThreadManager::Release();

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("Core-Tests"), RunCoreTests);
