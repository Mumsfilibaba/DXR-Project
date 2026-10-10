#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Threading/ThreadManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "RHIBootTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

#if PLATFORM_MACOS
struct FMetalDebugLayerArmer
{
    FMetalDebugLayerArmer()
    {
        FPlatformMisc::PrepareMetalDebugLayerEnvironment(true);
    }
};

static FMetalDebugLayerArmer GMetalDebugLayerArmer;
#endif

static int32 RunRHIBootTests()
{
    TestHarness::Initialize("TestResults_RHIBoot.log");
    LOG_INFO("=== RHI Boot Tests ===");

    if (!FThreadManager::Initialize())
    {
        LOG_ERROR("Failed to initialize the thread manager");
        TestHarness::Shutdown();
        return -1;
    }

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    RUN_TEST("RHIBoot_Null", RHIBoot_Null_Test());

#if PLATFORM_MACOS
    RUN_TEST("RHIBoot_Vulkan", RHIBoot_Vulkan_Test());
    RUN_TEST("RHIBoot_Metal", RHIBoot_Metal_Test());
    RUN_TEST("RHIBoot_MetalResidencyFallback", RHIBoot_MetalResidencyFallback_Test());
#elif PLATFORM_WINDOWS
    RUN_TEST("RHIBoot_D3D12", RHIBoot_D3D12_Test());
    RUN_TEST("RHIBoot_Vulkan", RHIBoot_Vulkan_Test());
#endif

    FTaskGraph::Release();
    FThreadManager::Release();

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN("RHI-Boot-Tests", RunRHIBootTests);
