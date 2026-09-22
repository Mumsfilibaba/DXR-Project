#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Threading/ThreadManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "RHIBootTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

#if PLATFORM_MACOS
    FPlatformMisc::PrepareMetalDebugLayerEnvironment(true);
#endif

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
