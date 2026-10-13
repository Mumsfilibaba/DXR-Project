#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Misc/CommandLine.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "AssetFileTests.h"
#include "ContentMountsTests.h"
#include "AssetRegistryTests.h"
#include "AssetRefTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunAssetCoreTests()
{
    TestHarness::Initialize(TEXT("TestResults_AssetCore.log"));
    LOG_INFO("=== AssetCore Tests ===");

    RUN_TEST(TEXT("AssetFile"), AssetFile_Test());
    RUN_TEST(TEXT("ContentMounts"), ContentMounts_Test());
    RUN_TEST(TEXT("AssetRegistry"), AssetRegistry_Test());
    RUN_TEST(TEXT("AssetRef"), AssetRef_Test());

    if (CommandLine::FindOption(TEXT("LargeFiles")))
    {
        RUN_TEST(TEXT("AssetFileLarge"), AssetFileLarge_Test());
    }

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("AssetCore-Tests"), RunAssetCoreTests);
