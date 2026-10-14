#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "EngineContentTests.h"
#include "ImportSceneBakingTests.h"
#include "MeshStreamLayoutTests.h"
#include "PrimitiveShapeTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunEngineTests()
{
    TestHarness::Initialize(TEXT("TestResults_Engine.log"));
    LOG_INFO("=== Engine Tests ===");

    RUN_TEST(TEXT("EngineContent"), EngineContent_Test());
    RUN_TEST(TEXT("ImportSceneBaking"), ImportSceneBaking_Test());
    RUN_TEST(TEXT("MeshStreamLayouts"), MeshStreamLayouts_Test());
    RUN_TEST(TEXT("PrimitiveShapes"), PrimitiveShapes_Test());
    RUN_TEST(TEXT("BackgroundTasks"), BackgroundTasks_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("Engine-Tests"), RunEngineTests);
