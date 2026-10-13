#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Threading/ThreadManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "AssetFileTests.h"
#include "ContentMountsTests.h"
#include "AssetRegistryTests.h"
#include "AssetRefTests.h"
#include "AssetLoaderTests.h"
#include "AssetImporterTests.h"
#include "TextureAssetTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunAssetCoreTests()
{
    TestHarness::Initialize(TEXT("TestResults_AssetCore.log"));
    LOG_INFO("=== AssetCore Tests ===");

    FThreadManager::Initialize();

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    RUN_TEST(TEXT("AssetFile"), AssetFile_Test());
    RUN_TEST(TEXT("ContentMounts"), ContentMounts_Test());
    RUN_TEST(TEXT("AssetRegistry"), AssetRegistry_Test());
    RUN_TEST(TEXT("AssetRef"), AssetRef_Test());
    RUN_TEST(TEXT("AssetLoader"), AssetLoader_Test());
    RUN_TEST(TEXT("AssetImporter"), AssetImporter_Test());
    RUN_TEST(TEXT("TextureAsset"), TextureAsset_Test());

    if (CommandLine::FindOption(TEXT("LargeFiles")))
    {
        RUN_TEST(TEXT("AssetFileLarge"), AssetFileLarge_Test());
    }

    FTaskGraph::Release();
    FThreadManager::Release();

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("AssetCore-Tests"), RunAssetCoreTests);
