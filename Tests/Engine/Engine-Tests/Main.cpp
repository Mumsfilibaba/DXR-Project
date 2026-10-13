#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "MaterialAssetTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunEngineTests()
{
    TestHarness::Initialize(TEXT("TestResults_Engine.log"));
    LOG_INFO("=== Engine Tests ===");

    RUN_TEST(TEXT("MaterialAsset"), MaterialAsset_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("Engine-Tests"), RunEngineTests);
